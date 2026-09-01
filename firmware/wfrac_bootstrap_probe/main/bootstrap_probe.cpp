#include "bootstrap_probe.hpp"

#include <array>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

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

#ifndef CONFIG_WFRAC_PROBE_ENABLED
#define CONFIG_WFRAC_PROBE_ENABLED 0
#endif
#ifndef CONFIG_WFRAC_PROBE_WIFI_SSID
#define CONFIG_WFRAC_PROBE_WIFI_SSID ""
#endif
#ifndef CONFIG_WFRAC_PROBE_WIFI_PASSWORD
#define CONFIG_WFRAC_PROBE_WIFI_PASSWORD ""
#endif
#ifndef CONFIG_WFRAC_PROBE_HOST
#define CONFIG_WFRAC_PROBE_HOST ""
#endif
#ifndef CONFIG_WFRAC_PROBE_PORT
#define CONFIG_WFRAC_PROBE_PORT 51443
#endif
#ifndef CONFIG_WFRAC_PROBE_DEVICE_ID
#define CONFIG_WFRAC_PROBE_DEVICE_ID ""
#endif
#ifndef CONFIG_WFRAC_PROBE_OPERATOR_ID
#define CONFIG_WFRAC_PROBE_OPERATOR_ID ""
#endif
#ifndef CONFIG_WFRAC_PROBE_TIME_SEED
#define CONFIG_WFRAC_PROBE_TIME_SEED ""
#endif
#ifndef CONFIG_WFRAC_PROBE_SCHEME_HTTP
#define CONFIG_WFRAC_PROBE_SCHEME_HTTP 0
#endif
#ifndef CONFIG_WFRAC_PROBE_SERVER_CERT_PEM
#define CONFIG_WFRAC_PROBE_SERVER_CERT_PEM ""
#endif

namespace {

constexpr char kTag[] = "wfrac_probe";
constexpr EventBits_t kGotIpBit = BIT0;
constexpr EventBits_t kDisconnectedBit = BIT1;
constexpr TickType_t kWifiWaitTicks = pdMS_TO_TICKS(20000U);
constexpr uint32_t kDiagnosticTimeoutMs = 5000U;

constexpr std::size_t kHostCapacity = 128U;
constexpr std::size_t kDeviceIdCapacity = 128U;
constexpr std::size_t kOperatorIdCapacity = 128U;
constexpr std::size_t kRequestCapacity = 1024U;
constexpr std::size_t kResponseCapacity = 2048U;
constexpr std::size_t kAirconIdCapacity = 128U;
constexpr std::size_t kMacAddressCapacity = 64U;

void log_stage(const char* stage, const char* result) { ESP_LOGI(kTag, "%s %s", stage, result); }

struct ProbeConfig {
  std::array<char, kHostCapacity> host{};
  std::array<char, kDeviceIdCapacity> device_id{};
  std::array<char, kOperatorIdCapacity> operator_id{};
  wifi_config_t wifi{};
  int64_t time_seed = 0;
  uint16_t port = 0;
  wfrac_transport::SchemeKnowledge scheme = wfrac_transport::SchemeKnowledge::https_confirmed;
};

struct WifiEvents {
  EventGroupHandle_t group = nullptr;
  int disconnect_reason = 0;
};

bool is_nonempty(const char* value) { return value != nullptr && value[0] != '\0'; }

bool copy_text(const char* source, char* destination, std::size_t destination_size) {
  if (!is_nonempty(source) || destination == nullptr || destination_size == 0U) return false;
  const std::size_t source_size = std::strlen(source);
  if (source_size >= destination_size) return false;
  std::memcpy(destination, source, source_size);
  destination[source_size] = '\0';
  return true;
}

bool copy_wifi_text(const char* source, uint8_t* destination, std::size_t destination_size) {
  if (source == nullptr || destination == nullptr) return false;
  const std::size_t source_size = std::strlen(source);
  if (source_size > destination_size) return false;
  if (source_size > 0U) std::memcpy(destination, source, source_size);
  return true;
}

bool parse_positive_int64(const char* text, int64_t* value) {
  if (!is_nonempty(text) || value == nullptr) return false;
  int64_t parsed = 0;
  for (const char* cursor = text; *cursor != '\0'; ++cursor) {
    if (*cursor < '0' || *cursor > '9') return false;
    const int digit = *cursor - '0';
    if (parsed > (std::numeric_limits<int64_t>::max() - digit) / 10) return false;
    parsed = parsed * 10 + digit;
  }
  if (parsed <= 0) return false;
  *value = parsed;
  return true;
}

bool load_config(ProbeConfig* config) {
  if (config == nullptr) return false;
  *config = {};
  if (!copy_text(CONFIG_WFRAC_PROBE_HOST, config->host.data(), config->host.size()) ||
      !copy_text(CONFIG_WFRAC_PROBE_DEVICE_ID, config->device_id.data(), config->device_id.size()) ||
      !copy_text(CONFIG_WFRAC_PROBE_OPERATOR_ID, config->operator_id.data(), config->operator_id.size()) ||
      !is_nonempty(CONFIG_WFRAC_PROBE_WIFI_SSID) ||
      !parse_positive_int64(CONFIG_WFRAC_PROBE_TIME_SEED, &config->time_seed)) {
    return false;
  }

  const int configured_port = CONFIG_WFRAC_PROBE_PORT;
  if (configured_port < 1 || configured_port > 65535) return false;
  config->port = static_cast<uint16_t>(configured_port);

  if (!copy_wifi_text(CONFIG_WFRAC_PROBE_WIFI_SSID, config->wifi.sta.ssid, sizeof(config->wifi.sta.ssid)) ||
      !copy_wifi_text(CONFIG_WFRAC_PROBE_WIFI_PASSWORD, config->wifi.sta.password, sizeof(config->wifi.sta.password))) {
    return false;
  }

#if CONFIG_WFRAC_PROBE_SCHEME_HTTP
  config->scheme = wfrac_transport::SchemeKnowledge::http_confirmed;
#else
  config->scheme = wfrac_transport::SchemeKnowledge::https_confirmed;
#endif
  return true;
}

void wifi_event_handler(void* argument, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  auto* events = static_cast<WifiEvents*>(argument);
  if (events == nullptr || events->group == nullptr) return;
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    const auto* disconnected = static_cast<const wifi_event_sta_disconnected_t*>(event_data);
    events->disconnect_reason = disconnected == nullptr ? 0 : disconnected->reason;
    xEventGroupSetBits(events->group, kDisconnectedBit);
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(events->group, kGotIpBit);
  }
}

void run_request_path(const ProbeConfig& config, wfrac_time_espidf::EspSystemTimeProvider* time_provider) {
  std::array<uint8_t, kRequestCapacity> request_storage{};
  wfrac_api::ApiRequestView request{};
  const wfrac_api::ApiClientContext context{"1.0", config.device_id.data(), config.operator_id.data()};
  if (wfrac_api::build_get_device_info_request(context, time_provider, {request_storage.data(), request_storage.size()},
                                               &request) != wfrac_api::ApiRequestError::none) {
    log_stage("API_REQUEST", "FAIL");
    return;
  }

  wfrac_transport_espidf::EspHttpTransportConfig transport_config{};
  if (config.scheme == wfrac_transport::SchemeKnowledge::https_confirmed) {
    transport_config.tls.server_cert_pem = CONFIG_WFRAC_PROBE_SERVER_CERT_PEM;
    transport_config.tls.server_cert_len = 0U;
    if (!transport_config.tls.has_valid_pem_certificate()) {
      ESP_LOGE(kTag, "HTTPS trust certificate missing");
      return;
    }
  }

  wfrac_transport_espidf::EspHttpTransport transport(transport_config);
  wfrac_transport::SchemeCache scheme_cache{};
  scheme_cache.knowledge = config.scheme;
  std::array<uint8_t, kResponseCapacity> response_storage{};
  wfrac_transport::TransportResponse response{};
  wfrac_client::RequestExecutionReport report{};
  const wfrac_client::ClientEndpoint endpoint{config.host.data(), config.port, kDiagnosticTimeoutMs};
  const wfrac_transport::TransportError transport_error =
      wfrac_client::execute_api_request(request, endpoint, &scheme_cache, &transport,
                                        {response_storage.data(), response_storage.size()}, &response, &report);
  if (transport_error != wfrac_transport::TransportError::none) {
    ESP_LOGE(kTag, "TRANSPORT FAIL (%d)", static_cast<int>(transport_error));
    return;
  }

  ESP_LOGI(kTag, "HTTP_STATUS PASS (%d)", response.http_status);
  wfrac_api::ApiResponseEnvelopeView envelope{};
  if (wfrac_api::parse_api_response_envelope({response_storage.data(), response.body_size}, "getDeviceInfo", &envelope) !=
      wfrac_api::ApiResponseError::none) {
    ESP_LOGE(kTag, "API_ENVELOPE FAIL");
    return;
  }
  ESP_LOGI(kTag, "API_ENVELOPE PASS result=%" PRId64, envelope.result);
  if (!envelope.has_contents) return;

  std::array<uint8_t, kAirconIdCapacity> aircon_id{};
  std::array<uint8_t, kMacAddressCapacity> mac_address{};
  wfrac_api::GetDeviceInfoContents device_info{};
  if (wfrac_api::parse_get_device_info_contents(envelope.contents_json, {aircon_id.data(), aircon_id.size()},
                                                 {mac_address.data(), mac_address.size()}, &device_info) !=
      wfrac_api::ApiResponseError::none) {
    ESP_LOGE(kTag, "DEVICE_INFO FAIL");
    return;
  }
  ESP_LOGI(kTag, "DEVICE_INFO PASS airconId=%s", reinterpret_cast<const char*>(aircon_id.data()));
  if (device_info.has_mac_address) {
    ESP_LOGI(kTag, "DEVICE_INFO macAddress=%s", reinterpret_cast<const char*>(mac_address.data()));
  }
  if (device_info.has_ap_mode) ESP_LOGI(kTag, "DEVICE_INFO apMode=%" PRId64, device_info.ap_mode);
}

}  // namespace

void run_wfrac_bootstrap_probe() {
  log_stage("BOOT", "PASS");
#if !CONFIG_WFRAC_PROBE_ENABLED
  ESP_LOGI(kTag, "bootstrap probe disabled");
  return;
#else
  ProbeConfig config{};
  if (!load_config(&config)) {
    log_stage("CONFIG", "FAIL");
    return;
  }
  log_stage("CONFIG", "PASS");

  wfrac_time_espidf::EspSystemTimeProvider time_provider{};
  if (time_provider.seed({config.time_seed, wfrac_api::TimeQuality::estimated}) !=
      wfrac_time_espidf::EspSystemTimeError::none) {
    log_stage("TIME", "FAIL");
    return;
  }
  log_stage("TIME", "PASS");

  WifiEvents events{};
  events.group = xEventGroupCreate();
  if (events.group == nullptr) {
    log_stage("WIFI_INIT", "FAIL");
    return;
  }

  bool netif_created = false;
  bool wifi_initialized = false;
  bool wifi_started = false;
  bool got_ip = false;
  esp_event_handler_instance_t wifi_handler = nullptr;
  esp_event_handler_instance_t ip_handler = nullptr;
  esp_netif_t* station_netif = nullptr;

  esp_err_t error = esp_netif_init();
  if (error == ESP_OK && esp_event_loop_create_default() == ESP_OK) {
    station_netif = esp_netif_create_default_wifi_sta();
    netif_created = station_netif != nullptr;
  }
  wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
  if (!netif_created || esp_wifi_init(&wifi_init) != ESP_OK) goto cleanup;
  wifi_initialized = true;
  if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK || esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
      esp_wifi_set_config(WIFI_IF_STA, &config.wifi) != ESP_OK ||
      esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event_handler, &events,
                                          &wifi_handler) != ESP_OK ||
      esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, &events, &ip_handler) !=
          ESP_OK ||
      esp_wifi_start() != ESP_OK) {
    goto cleanup;
  }
  wifi_started = true;
  log_stage("WIFI_INIT", "PASS");
  if (esp_wifi_connect() != ESP_OK) goto cleanup;
  log_stage("WIFI_CONNECT_START", "PASS");

  const EventBits_t bits = xEventGroupWaitBits(events.group, kGotIpBit | kDisconnectedBit, pdTRUE, pdFALSE,
                                                kWifiWaitTicks);
  got_ip = (bits & kGotIpBit) != 0U;
  if (!got_ip) {
    if ((bits & kDisconnectedBit) != 0U) ESP_LOGE(kTag, "GOT_IP FAIL disconnect=%d", events.disconnect_reason);
    else log_stage("GOT_IP", "FAIL");
    goto cleanup;
  }
  log_stage("GOT_IP", "PASS");
  run_request_path(config, &time_provider);

cleanup:
  bool cleanup_ok = true;
  if (wifi_handler != nullptr) {
    cleanup_ok = esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_handler) == ESP_OK &&
                 cleanup_ok;
  }
  if (ip_handler != nullptr) {
    cleanup_ok = esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_handler) == ESP_OK && cleanup_ok;
  }
  if (wifi_started) {
    if (got_ip) cleanup_ok = esp_wifi_disconnect() == ESP_OK && cleanup_ok;
    cleanup_ok = esp_wifi_stop() == ESP_OK && cleanup_ok;
  }
  if (wifi_initialized) cleanup_ok = esp_wifi_deinit() == ESP_OK && cleanup_ok;
  if (station_netif != nullptr) esp_netif_destroy_default_wifi(station_netif);
  vEventGroupDelete(events.group);
  log_stage("CLEANUP", cleanup_ok ? "PASS" : "FAIL");
#endif
}
