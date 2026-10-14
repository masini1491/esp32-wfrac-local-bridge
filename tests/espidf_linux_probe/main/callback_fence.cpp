// Offline A6b feasibility smoke: real pinned SDK TCPIP task; no netif or DNS.
#include <cstdio>
#include <cstdlib>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/tcpip.h"

namespace {
StaticEventGroup_t event_storage;
EventGroupHandle_t events;
constexpr EventBits_t initialized = 1;
constexpr EventBits_t dispatched = 2;
void init_done(void*) { xEventGroupSetBits(events, initialized); }
void callback(void*) { xEventGroupSetBits(events, dispatched); }
void wait_for(EventBits_t bit, const char* label) {
    if ((xEventGroupWaitBits(events, bit, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000)) & bit) == 0) {
        std::printf("A6B FAIL timeout: %s\n", label);
        std::fflush(stdout);
        std::_Exit(1);
    }
}
}
extern "C" void app_main() {
    events = xEventGroupCreateStatic(&event_storage);
    tcpip_init(init_done, nullptr);
    wait_for(initialized, "tcpip_init");
    const err_t result = tcpip_try_callback(callback, nullptr);
    if (result != ERR_OK) {
        std::printf("A6B FAIL callback admission: %d\n", static_cast<int>(result));
        std::fflush(stdout);
        std::_Exit(1);
    }
    wait_for(dispatched, "tcpip_try_callback");
    std::puts("A6B SDK_SMOKE PASS: real TCPIP callback dispatched; network/DNS/TLS NOT_RUN");
    std::fflush(stdout);
    // Host process termination is not SDK worker teardown or token reclamation.
    std::_Exit(0);
}
