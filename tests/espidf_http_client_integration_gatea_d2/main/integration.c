#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_http_client.h"
#include "esp_transport.h"
#include "sdkconfig.h"
#if !CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS || !CONFIG_ESP_HTTP_CLIENT_ENABLE_CUSTOM_TRANSPORT
#error Required HTTPS and custom transport configuration missing
#endif
/* Build-only candidate. Execution requires independent zero-egress admission. */
typedef struct {
    size_t offset, bytes;
    unsigned connects, writes, closes, destroys;
    bool fail_close;
    unsigned first_close_ordinal;
    int first_close_return;
    char body[8];
} owner_t;
static const char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
static owner_t *owner(esp_transport_handle_t t) { return esp_transport_get_context_data(t); }
static int connect_memory(esp_transport_handle_t t, const char *h, int p, int ms)
{ (void)h; (void)p; (void)ms; owner(t)->connects++; return 0; }
static int read_memory(esp_transport_handle_t t, char *b, int n, int ms)
{
    (void)ms;
    owner_t *o = owner(t);
    size_t left = sizeof(response)-1-o->offset;
    size_t count = left < (size_t)n ? left : (size_t)n;
    memcpy(b, response+o->offset, count); o->offset += count; return (int)count;
}
static int write_memory(esp_transport_handle_t t, const char *b, int n, int ms)
{ (void)b; (void)ms; owner(t)->writes++; return n; }
static int close_memory(esp_transport_handle_t t)
{
    owner_t *o = owner(t);
    int result = o->fail_close ? -1 : 0;
    ++o->closes;
    if (o->closes == 1) {
        o->first_close_ordinal = o->closes;
        o->first_close_return = result;
    }
    return result;
}
static int poll_memory(esp_transport_handle_t t, int ms) { (void)t; (void)ms; return 1; }
static int destroy_memory(esp_transport_handle_t t) { owner(t)->destroys++; return 0; }
static esp_err_t event_memory(esp_http_client_event_t *e)
{
    owner_t *o = e->user_data;
    if (e->event_id == HTTP_EVENT_ON_DATA) {
        if (e->data_len < 0 || o->bytes+(size_t)e->data_len > sizeof(o->body)) return ESP_FAIL;
        memcpy(o->body+o->bytes, e->data, (size_t)e->data_len); o->bytes += (size_t)e->data_len;
    }
    return ESP_OK;
}
static bool witness(const char *url, unsigned kind)
{
    owner_t o = {.fail_close = kind != 0};
    esp_transport_handle_t t = esp_transport_init();
    if (!t) return false;
    esp_transport_set_context_data(t, &o);
    esp_transport_set_default_port(t, strstr(url, "https:") ? 443 : 80);
    esp_transport_set_func(t, connect_memory, read_memory, write_memory, close_memory,
                           poll_memory, poll_memory, destroy_memory);
    esp_http_client_config_t cfg = {.url=url, .transport=t, .timeout_ms=50,
        .event_handler=event_memory, .user_data=&o, .disable_auto_redirect=true,
        .max_authorization_retries=0};
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) { esp_transport_destroy(t); return false; }
    esp_err_t performed = esp_http_client_perform(c);
    int status = esp_http_client_get_status_code(c);
    esp_err_t closed = esp_http_client_close(c);
    esp_err_t cleaned = esp_http_client_cleanup(c);
    /* Caller owns injected handle; SDK list ownership must not consume it. */
    bool parsed = performed == ESP_OK && status == 200 && o.bytes == 2 &&
                  memcmp(o.body, "OK", 2) == 0 && o.connects == 1 && o.writes > 0;
    bool retained = o.destroys == 0;
    bool close_seen = o.closes > 0 && o.first_close_ordinal == 1;
    bool observed_first_close_failure = close_seen && o.first_close_return == -1;
    /* Negative oracle only: cleanup success cannot erase an observed callback
     * failure. There is no physical-close/quiescence signal in this fixture. */
    bool invalid_cleanup_implies_quiescence = cleaned == ESP_OK;
    bool reject_invalid_oracle = invalid_cleanup_implies_quiescence &&
                                 observed_first_close_failure;
    bool ok = parsed && retained && close_seen;
    if (kind == 0) ok = ok && closed == ESP_OK && cleaned == ESP_OK;
    if (kind == 1) ok = ok && observed_first_close_failure;
    if (kind == 2) ok = ok && reject_invalid_oracle;
    printf("{\"phase\":\"oracle\",\"kind\":%u,\"first_close_ordinal\":%u,"
           "\"first_close_return\":%d,\"explicit_close_return\":%d,"
           "\"cleanup_return\":%d,\"quiescence\":\"UNPROVEN\"}\n",
           kind, o.first_close_ordinal, o.first_close_return, (int)closed, (int)cleaned);
    esp_transport_destroy(t);
    return ok && o.destroys == 1;
}
void app_main(void)
{
    const char *urls[] = {"http://fixture.invalid/", "https://fixture.invalid/"};
    unsigned passed = 0;
    for (unsigned s=0; s<2; ++s) for (unsigned k=0; k<3; ++k) {
        bool ok = witness(urls[s], k);
        printf("{\"scheme\":%u,\"witness\":%u,\"result\":\"%s\"}\n", s, k, ok ? "PASS" : "FAIL");
        passed += ok;
    }
    printf("{\"total\":6,\"passed\":%u,\"failed\":%u}\n", passed, 6-passed);
    fflush(NULL);
    exit(passed == 6 ? EXIT_SUCCESS : EXIT_FAILURE);
}
