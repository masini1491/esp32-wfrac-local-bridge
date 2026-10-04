#include "bootstrap_probe.hpp"
#include "sdkconfig.h"

#if CONFIG_WFRAC_PROBE_WEBUI_ENABLED
#include <array>
#include <cstdio>
#include <cstring>
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"

namespace {
// HTTPD serializes handlers. No project worker, queue, polling, or shared heap.
std::array<char, diagnostic::kFormLimit> body{};
diagnostic::Input input{};
std::array<char, 33> nonce{};
bool nonce_valid = false, healthy = true;
constexpr char kPage[] = R"HTML(<!doctype html><meta charset=utf-8><title>WF-RAC diagnostic</title>
<h1>Temporary read-only diagnostic</h1><p>Not production provisioning. Use an isolated lab and a unique AP passphrase.
Inputs travel over local HTTP protected only by the WPA2 lab link. This does not prove hardware operation.</p>
<form method=post action=/probe autocomplete=off>
<label>STA SSID <input name=ssid maxlength=32 required></label><br>
<label>STA password <input name=password type=password maxlength=64></label><br>
<label>WF-RAC numeric IPv4 <input name=host maxlength=128 required></label><br>
<label>Port <input name=port value=51443 required></label><br>
<label>Device ID <input name=device_id type=password maxlength=128 required></label><br>
<label>Operator ID <input name=operator_id type=password maxlength=128 required></label><br>
<label>Estimated Unix seconds <input name=time_seed required></label><br>
<label>Explicit scheme <select name=scheme><option value=https>HTTPS</option><option value=http>HTTP</option></select></label><br>
<label>HTTPS PEM trust certificate <textarea name=certificate maxlength=4096></textarea></label><br>
<label>Optional verified server name <input name=common_name maxlength=128></label><br>)HTML";

void headers(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");
  httpd_resp_set_hdr(req, "Referrer-Policy", "no-referrer");
  httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
  httpd_resp_set_hdr(req, "Content-Security-Policy", "default-src 'none'; form-action 'self'; frame-ancestors 'none'; base-uri 'none'");
}
esp_err_t reject(httpd_req_t* req, const char* status) {
  headers(req);
  httpd_resp_set_status(req, status);
  return httpd_resp_send(req, "<p>Diagnostic submission rejected; no probe started.</p>", HTTPD_RESP_USE_STRLEN);
}
bool header_equals(httpd_req_t* req, const char* name, const char* expected) {
  char value[96]{};
  return httpd_req_get_hdr_value_len(req, name) < sizeof(value) &&
      httpd_req_get_hdr_value_str(req, name, value, sizeof(value)) == ESP_OK && std::strcmp(value, expected) == 0;
}
bool ap_only(httpd_req_t* req) {
  sockaddr_in local{};
  socklen_t size = sizeof(local);
  // Reject requests arriving through the STA address; never expose the UI to the joined LAN.
  return std::strchr(req->uri, '?') == nullptr &&
      getsockname(httpd_req_to_sockfd(req), reinterpret_cast<sockaddr*>(&local), &size) == 0 &&
      local.sin_family == AF_INET && ntohl(local.sin_addr.s_addr) == 0xc0a80401U &&
      (header_equals(req, "Host", "192.168.4.1") || header_equals(req, "Host", "192.168.4.1:80"));
}
esp_err_t form(httpd_req_t* req) {
  if (!ap_only(req) || !healthy) return reject(req, "403 Forbidden");
  std::array<uint8_t, 16> random{};
  esp_fill_random(random.data(), random.size());
  constexpr char hex[] = "0123456789abcdef";
  for (std::size_t i = 0; i < random.size(); ++i) {
    nonce[2 * i] = hex[random[i] >> 4]; nonce[2 * i + 1] = hex[random[i] & 15];
  }
  nonce_valid = true;
  headers(req);
  char hidden[128]{};
  std::snprintf(hidden, sizeof(hidden), "<input type=hidden name=nonce value='%s'><button>Run one probe</button></form>", nonce.data());
  if (httpd_resp_send_chunk(req, kPage, sizeof(kPage) - 1) != ESP_OK ||
      httpd_resp_send_chunk(req, hidden, std::strlen(hidden)) != ESP_OK) return ESP_FAIL;
  return httpd_resp_send_chunk(req, nullptr, 0);
}
esp_err_t submit(httpd_req_t* req) {
  diagnostic::EraseOnExit wipe_body{body.data(), body.size()};
  diagnostic::EraseOnExit wipe_input{&input, sizeof(input)};
  if (!ap_only(req) || !healthy || !nonce_valid ||
      !(header_equals(req, "Origin", "http://192.168.4.1") || header_equals(req, "Origin", "http://192.168.4.1:80")) ||
      !header_equals(req, "Content-Type", "application/x-www-form-urlencoded")) return reject(req, "403 Forbidden");
  if (req->content_len == 0 || req->content_len > body.size()) return reject(req, "413 Content Too Large");
  const int64_t deadline = esp_timer_get_time() + 5000000;  // Monotonic receive bound, NOT Unix epoch.
  std::size_t received = 0;
  while (received < req->content_len) {
    if (esp_timer_get_time() >= deadline) return reject(req, "408 Request Timeout");
    const int n = httpd_req_recv(req, body.data() + received, req->content_len - received);
    if (n <= 0) return reject(req, "408 Request Timeout");  // No receive-timeout retry.
    received += static_cast<std::size_t>(n);
  }
  if (diagnostic::parse_form(body.data(), received, &input) != diagnostic::InputError::none ||
      std::strcmp(input.nonce.data(), nonce.data()) != 0) return reject(req, "400 Bad Request");
  nonce_valid = false;  // Burn before any clock/Wi-Fi/request side effect; replay cannot repeat a probe.
  diagnostic::erase(nonce.data(), nonce.size());
  const auto result = execute_diagnostic_probe(input, true);
  healthy = result.cleanup_ok;  // Failed cleanup forbids another submission.
  char html[640]{};
  if (!diagnostic::format_result(result, html, sizeof(html))) return reject(req, "500 Internal Server Error");
  headers(req);
  ESP_LOGI("wfrac_probe", "stage=%u code=%d cleanup=%d", static_cast<unsigned>(result.stage), result.code, result.cleanup_ok);
  return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}
}  // namespace

void start_diagnostic_webui() {
  // Silence SDK diagnostics that could include submitted endpoint/certificate data.
  esp_log_level_set("*", ESP_LOG_NONE);
  esp_log_level_set("wfrac_probe", ESP_LOG_INFO);
  // A local, unique AP passphrase is mandatory. Empty repository defaults fail closed.
  const char* passphrase = CONFIG_WFRAC_PROBE_AP_PASSWORD;
  const auto length = std::strlen(passphrase);
  if (length < 8 || length > 63) { ESP_LOGE("wfrac_probe", "diagnostic AP configuration missing/invalid"); return; }
  esp_netif_t* ap = nullptr;
  esp_netif_t* sta = nullptr;
  httpd_handle_t server = nullptr;
  bool loop = false, initialized = false, started = false;
  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  init.nvs_enable = 0;
  wifi_config_t config{};
  diagnostic::EraseOnExit wipe{&config, sizeof(config)};
  constexpr char ssid[] = "WFRAC-Diagnostic";
  std::memcpy(config.ap.ssid, ssid, sizeof(ssid) - 1);
  config.ap.ssid_len = sizeof(ssid) - 1;
  std::memcpy(config.ap.password, passphrase, length);
  config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  config.ap.max_connection = 1;
  httpd_config_t http = HTTPD_DEFAULT_CONFIG();
  http.stack_size = 8192;
  http.recv_wait_timeout = 1;
  http.send_wait_timeout = 2;
  http.max_open_sockets = 2;
  http.lru_purge_enable = true;
  httpd_uri_t get{};
  get.uri = "/"; get.method = HTTP_GET; get.handler = form;
  httpd_uri_t post{};
  post.uri = "/probe"; post.method = HTTP_POST; post.handler = submit;
  esp_log_level_set("wifi", ESP_LOG_NONE);
  esp_log_level_set("httpd_uri", ESP_LOG_NONE);
  esp_log_level_set("httpd_parse", ESP_LOG_NONE);
  if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK) goto fail;
  loop = true;
  ap = esp_netif_create_default_wifi_ap();
  sta = esp_netif_create_default_wifi_sta();
  if (ap == nullptr || sta == nullptr || esp_wifi_init(&init) != ESP_OK) goto fail;
  initialized = true;
  if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK || esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK ||
      esp_wifi_set_config(WIFI_IF_AP, &config) != ESP_OK || esp_wifi_start() != ESP_OK) goto fail;
  started = true;
  if (httpd_start(&server, &http) != ESP_OK || httpd_register_uri_handler(server, &get) != ESP_OK ||
      httpd_register_uri_handler(server, &post) != ESP_OK) goto fail;
  ESP_LOGI("wfrac_probe", "diagnostic UI ready; explicit submission required");
  return;
fail:
  if (server != nullptr) httpd_stop(server);
  if (started) esp_wifi_stop();
  if (initialized) esp_wifi_deinit();
  if (sta != nullptr) esp_netif_destroy_default_wifi(sta);
  if (ap != nullptr) esp_netif_destroy_default_wifi(ap);
  if (loop) esp_event_loop_delete_default();
  ESP_LOGE("wfrac_probe", "diagnostic UI initialization failed");
}
#else
void start_diagnostic_webui() {}
#endif
