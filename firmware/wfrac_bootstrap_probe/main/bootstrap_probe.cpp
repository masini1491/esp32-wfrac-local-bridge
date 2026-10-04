#include "bootstrap_probe.hpp"

#include <array>
#include <cstring>
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "wfrac_api/api_request.hpp"
#include "wfrac_api/api_response.hpp"
#include "wfrac_client/request_executor.hpp"
#include "wfrac_time_espidf/esp_system_time_provider.hpp"
#include "wfrac_transport/scheme_policy.hpp"
#include "wfrac_transport_espidf/esp_http_transport.hpp"

namespace {
constexpr EventBits_t kGotIp = BIT0, kDisconnected = BIT1;
struct WifiEvents { EventGroupHandle_t group; int reason = 0; };
void event_handler(void* argument, esp_event_base_t base, int32_t id, void* data) {
  auto* events = static_cast<WifiEvents*>(argument);
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    const auto* event = static_cast<const wifi_event_sta_disconnected_t*>(data);
    events->reason = event == nullptr ? 0 : event->reason;
    xEventGroupSetBits(events->group, kDisconnected);
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) xEventGroupSetBits(events->group, kGotIp);
}

void request_path(const diagnostic::Input& input, wfrac_api::ITimeProvider* clock, diagnostic::Result* result) {
  std::array<uint8_t, 1024> request_bytes{};
  std::array<uint8_t, 2048> response_bytes{};
  std::array<uint8_t, 128> aircon_id{};
  std::array<uint8_t, 64> mac{};
  diagnostic::EraseOnExit wipe_request{request_bytes.data(), request_bytes.size()};
  diagnostic::EraseOnExit wipe_response{response_bytes.data(), response_bytes.size()};
  diagnostic::EraseOnExit wipe_id{aircon_id.data(), aircon_id.size()};
  diagnostic::EraseOnExit wipe_mac{mac.data(), mac.size()};
  result->stage = diagnostic::Stage::request;
  const wfrac_api::ApiClientContext context{"1.0", input.device_id.data(), input.operator_id.data()};
  wfrac_api::ApiRequestView request{};
  const auto request_error = wfrac_api::build_get_device_info_request(context, clock,
      {request_bytes.data(), request_bytes.size()}, &request);
  if (request_error != wfrac_api::ApiRequestError::none) { result->code = static_cast<int>(request_error); return; }
  wfrac_transport_espidf::EspHttpTransportConfig config{};
  if (input.scheme == diagnostic::Scheme::https) {
    config.tls.server_cert_pem = input.certificate.data();
    config.tls.server_cert_len = std::strlen(input.certificate.data()) + 1;
    config.tls.common_name = input.common_name[0] == '\0' ? nullptr : input.common_name.data();
    if (!config.tls.has_valid_pem_certificate()) { result->code = static_cast<int>(diagnostic::InputError::trust_required); return; }
  }
  wfrac_transport_espidf::EspHttpTransport transport{config};
  wfrac_transport::SchemeCache cache{};
  // Explicit configuration, NOT discovered evidence; sticky knowledge prevents fallback.
  cache.knowledge = input.scheme == diagnostic::Scheme::http ? wfrac_transport::SchemeKnowledge::http_confirmed :
      wfrac_transport::SchemeKnowledge::https_confirmed;
  wfrac_transport::TransportResponse response{};
  wfrac_client::RequestExecutionReport report{};
  const wfrac_client::ClientEndpoint endpoint{input.host.data(), input.port, 5000U};
  result->stage = diagnostic::Stage::transport;
  const auto error = wfrac_client::execute_api_request(request, endpoint, &cache, &transport,
      {response_bytes.data(), response_bytes.size()}, &response, &report);
  result->request_attempts = static_cast<unsigned>(report.attempts);
  result->code = static_cast<int>(error);
  result->http_status = response.http_status;
  if (error != wfrac_transport::TransportError::none) return;
  result->stage = diagnostic::Stage::envelope;
  wfrac_api::ApiResponseEnvelopeView envelope{};
  const auto parsed = wfrac_api::parse_api_response_envelope({response_bytes.data(), response.body_size}, "getDeviceInfo", &envelope);
  result->code = static_cast<int>(parsed);
  if (parsed != wfrac_api::ApiResponseError::none) return;
  result->has_result = true;
  result->api_result = envelope.result;  // Raw command-specific integer, not a global success assertion.
  if (!envelope.has_contents) return;
  result->stage = diagnostic::Stage::device_info;
  wfrac_api::GetDeviceInfoContents contents{};
  const auto typed = wfrac_api::parse_get_device_info_contents(envelope.contents_json,
      {aircon_id.data(), aircon_id.size()}, {mac.data(), mac.size()}, &contents);
  result->code = static_cast<int>(typed);
  result->device_info_parsed = typed == wfrac_api::ApiResponseError::none;
  if (result->device_info_parsed) result->stage = diagnostic::Stage::done;
  // Neither response identities nor request/credential strings enter logs/results.
}
}  // namespace

diagnostic::Result execute_diagnostic_probe(const diagnostic::Input& input, bool reuse_ap) {
  diagnostic::Result result{};
  const auto valid = diagnostic::validate_input(input);
  if (valid != diagnostic::InputError::none) { result.code = static_cast<int>(valid); return result; }
  result.stage = diagnostic::Stage::time;
  wfrac_time_espidf::EspSystemTimeProvider clock{};
  const auto seeded = clock.seed({input.time_seed, wfrac_api::TimeQuality::estimated});
  if (seeded != wfrac_time_espidf::EspSystemTimeError::none) { result.code = static_cast<int>(seeded); return result; }
  result.stage = diagnostic::Stage::wifi_init;
  WifiEvents events{xEventGroupCreate()};
  if (events.group == nullptr) { result.code = ESP_ERR_NO_MEM; return result; }
  bool initialized = false, started = false, connect_called = false, loop_owned = false;
  esp_netif_t* sta = nullptr;
  esp_event_handler_instance_t wifi_handler = nullptr, ip_handler = nullptr;
  wifi_config_t station{};
  diagnostic::EraseOnExit wipe_station{&station, sizeof(station)};
  std::memcpy(station.sta.ssid, input.ssid.data(), std::strlen(input.ssid.data()));
  std::memcpy(station.sta.password, input.password.data(), std::strlen(input.password.data()));
  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  init.nvs_enable = 0;
  auto operation = [&result](esp_err_t error) { result.code = error; return error == ESP_OK; };
  esp_log_level_set("wifi", ESP_LOG_NONE);
  esp_log_level_set("HTTP_CLIENT", ESP_LOG_NONE);
  esp_log_level_set("esp-tls", ESP_LOG_NONE);
  esp_log_level_set("transport_base", ESP_LOG_NONE);
  if (!reuse_ap) {
    if (!operation(esp_netif_init()) || !operation(esp_event_loop_create_default())) goto cleanup;
    loop_owned = true;
    sta = esp_netif_create_default_wifi_sta();
    if (sta == nullptr) { result.code = ESP_ERR_NO_MEM; goto cleanup; }
    if (!operation(esp_wifi_init(&init))) goto cleanup;
    initialized = true;
  }
  if (!operation(esp_wifi_set_storage(WIFI_STORAGE_RAM)) ||
      !operation(esp_wifi_set_mode(reuse_ap ? WIFI_MODE_APSTA : WIFI_MODE_STA)) ||
      !operation(esp_wifi_set_config(WIFI_IF_STA, &station)) ||
      !operation(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, event_handler, &events, &wifi_handler)) ||
      !operation(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, &events, &ip_handler))) goto cleanup;
  if (!reuse_ap) {
    if (!operation(esp_wifi_start())) goto cleanup;
    started = true;
  }
  result.stage = diagnostic::Stage::wifi_connect;
  result.connect_attempts = 1;
  connect_called = true;
  if (!operation(esp_wifi_connect())) goto cleanup;  // Exactly one; handlers never reconnect.
  result.stage = diagnostic::Stage::got_ip;
  {
    const auto bits = xEventGroupWaitBits(events.group, kGotIp | kDisconnected, pdTRUE, pdFALSE, pdMS_TO_TICKS(20000U));
    if ((bits & kDisconnected) != 0) { result.code = events.reason; goto cleanup; }
    if ((bits & kGotIp) == 0) { result.code = ESP_ERR_TIMEOUT; goto cleanup; }
  }
  request_path(input, &clock, &result);
cleanup:
  if (connect_called) {
    const auto disconnected = esp_wifi_disconnect();
    result.cleanup_ok = (disconnected == ESP_OK || disconnected == ESP_ERR_WIFI_NOT_CONNECT) && result.cleanup_ok;
  }
  if (reuse_ap) {
    wifi_config_t empty{};
    result.cleanup_ok = esp_wifi_set_config(WIFI_IF_STA, &empty) == ESP_OK && result.cleanup_ok;
    result.cleanup_ok = esp_wifi_set_mode(WIFI_MODE_AP) == ESP_OK && result.cleanup_ok;
  } else {
    if (started) result.cleanup_ok = esp_wifi_stop() == ESP_OK && result.cleanup_ok;
    if (initialized) result.cleanup_ok = esp_wifi_deinit() == ESP_OK && result.cleanup_ok;
  }
  if (wifi_handler != nullptr) result.cleanup_ok = esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_handler) == ESP_OK && result.cleanup_ok;
  if (ip_handler != nullptr) result.cleanup_ok = esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_handler) == ESP_OK && result.cleanup_ok;
  if (sta != nullptr) esp_netif_destroy_default_wifi(sta);
  if (loop_owned) result.cleanup_ok = esp_event_loop_delete_default() == ESP_OK && result.cleanup_ok;
  vEventGroupDelete(events.group);
  return result;
}

void run_wfrac_bootstrap_probe() {
#if CONFIG_WFRAC_PROBE_WEBUI_ENABLED
  start_diagnostic_webui();
#elif CONFIG_WFRAC_PROBE_ENABLED
  static diagnostic::Input input{};
  diagnostic::EraseOnExit wipe{&input, sizeof(input)};
  auto copy = [](const char* source, auto& out) {
    const auto n = std::strlen(source);
    if (n >= out.size()) return false;
    std::memcpy(out.data(), source, n + 1);
    return true;
  };
  if (!copy(CONFIG_WFRAC_PROBE_WIFI_SSID, input.ssid) || !copy(CONFIG_WFRAC_PROBE_WIFI_PASSWORD, input.password) ||
      !copy(CONFIG_WFRAC_PROBE_HOST, input.host) || !copy(CONFIG_WFRAC_PROBE_DEVICE_ID, input.device_id) ||
      !copy(CONFIG_WFRAC_PROBE_OPERATOR_ID, input.operator_id) ||
      !copy(CONFIG_WFRAC_PROBE_SERVER_CERT_PEM, input.certificate) ||
      !diagnostic::positive_decimal(CONFIG_WFRAC_PROBE_TIME_SEED, &input.time_seed)) {
    ESP_LOGE("wfrac_probe", "CONFIG FAIL"); return;
  }
  input.port = CONFIG_WFRAC_PROBE_PORT;
#if CONFIG_WFRAC_PROBE_SCHEME_HTTP
  input.scheme = diagnostic::Scheme::http;
#endif
  const auto result = execute_diagnostic_probe(input, false);
  ESP_LOGI("wfrac_probe", "stage=%u code=%d cleanup=%d", static_cast<unsigned>(result.stage), result.code, result.cleanup_ok);
#else
  ESP_LOGI("wfrac_probe", "bootstrap probe disabled");
#endif
}
