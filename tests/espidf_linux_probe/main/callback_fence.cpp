// A6b: real pinned SDK Linux TCPIP task, in-memory callbacks only.
// No netif, resolver request, sockets, TLS, token reuse or SDK source patch.
#include <cstdio>
#include <cstdlib>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/opt.h"
#include "lwip/tcpip.h"

#if !CONFIG_IDF_TARGET_LINUX
#error "A6b executes only on the isolated Linux host target"
#endif
#if LWIP_TCPIP_CORE_LOCKING
#error "This probe requires the independently reported serialized TCPIP configuration"
#endif

namespace {
constexpr EventBits_t initialized = 1;
constexpr EventBits_t completed = 2;
constexpr EventBits_t waiting = 4;
constexpr EventBits_t drained = 8;
constexpr TickType_t budget = pdMS_TO_TICKS(5000);
StaticEventGroup_t event_storage;
EventGroupHandle_t events;
StaticSemaphore_t release_storage;
SemaphoreHandle_t release_gate;
TaskHandle_t core_task;

enum class Event { enter, admitted, exit_marker, fence, saturated, drain };
struct Owner {
    Event order[6]{};
    unsigned count = 0;
    bool valid = true;
    bool retained = true;
    bool callback_active = false;
    bool expired = false;
    bool late_discarded = false;
    bool delayed = false;
    err_t fence_result = ERR_VAL;
    err_t admission_failure = ERR_VAL;
    unsigned queued = 0;
    unsigned consumed = 0;
    unsigned rejected_called = 0;
};
// Never reset, free or recycle these owners. Host exit is not SDK teardown.
Owner normal_owner;
Owner delayed_owner;
Owner full_owner;

[[noreturn]] void fail(const char* label) {
    std::printf("A6B FAIL: %s\n", label);
    std::fflush(stdout);
    std::_Exit(1);
}
void check(bool value, const char* label) { if (!value) fail(label); }
void wait_for(EventBits_t bit, const char* label) {
    check((xEventGroupWaitBits(events, bit, pdFALSE, pdTRUE, budget) & bit) == bit, label);
}
void record(Owner& owner, Event value) {
    if (xTaskGetCurrentTaskHandle() != core_task || owner.count >= 6) {
        owner.valid = false;
        return;
    }
    owner.order[owner.count++] = value;
}
void init_done(void*) {
    core_task = xTaskGetCurrentTaskHandle();
    xEventGroupSetBits(events, initialized);
}
void fence(void* argument) {
    auto& owner = *static_cast<Owner*>(argument);
    record(owner, Event::fence);
    owner.valid = owner.valid && !owner.callback_active && owner.retained;
    xEventGroupSetBits(events, completed);
}
void callback(void* argument) {
    auto& owner = *static_cast<Owner*>(argument);
    owner.callback_active = true;
    record(owner, Event::enter);
    owner.late_discarded = owner.expired;
    owner.fence_result = tcpip_try_callback(fence, &owner);
    record(owner, Event::admitted);
    if (owner.delayed) {
        xEventGroupSetBits(events, waiting);
        if (xSemaphoreTake(release_gate, budget) != pdTRUE) owner.valid = false;
    }
    owner.callback_active = false;
    record(owner, Event::exit_marker);
    if (owner.fence_result != ERR_OK) xEventGroupSetBits(events, completed);
}
void rejected(void* argument) {
    ++static_cast<Owner*>(argument)->rejected_called;
}
void filler(void* argument) {
    auto& owner = *static_cast<Owner*>(argument);
    ++owner.consumed;
    if (owner.consumed == owner.queued) {
        record(owner, Event::drain);
        xEventGroupSetBits(events, drained);
    }
}
void saturation(void* argument) {
    auto& owner = *static_cast<Owner*>(argument);
    // The TCPIP task cannot drain its own mailbox until this callback returns.
    // ERR_MEM can originate from message allocation or mailbox admission.
    for (unsigned i = 0; i <= TCPIP_MBOX_SIZE; ++i) {
        const err_t result = tcpip_try_callback(filler, &owner);
        if (result != ERR_OK) break;
        ++owner.queued;
    }
    owner.admission_failure = tcpip_try_callback(rejected, &owner);
    owner.fence_result = tcpip_try_callback(fence, &owner);
    record(owner, Event::saturated);
    if (owner.queued == 0) xEventGroupSetBits(events, drained);
}
void verify_order(const Owner& owner, const char* label) {
    check(owner.valid && owner.count == 4 &&
          owner.order[0] == Event::enter && owner.order[1] == Event::admitted &&
          owner.order[2] == Event::exit_marker && owner.order[3] == Event::fence &&
          owner.fence_result == ERR_OK && owner.retained, label);
}
}
extern "C" void app_main() {
    events = xEventGroupCreateStatic(&event_storage);
    release_gate = xSemaphoreCreateBinaryStatic(&release_storage);
    check(events != nullptr && release_gate != nullptr, "test synchronization allocation");
    tcpip_init(init_done, nullptr);
    wait_for(initialized, "tcpip_init timeout");
    check(core_task != xTaskGetCurrentTaskHandle(), "TCPIP task must differ from app task");

    check(tcpip_try_callback(callback, &normal_owner) == ERR_OK, "normal admission");
    wait_for(completed, "normal fence timeout");
    verify_order(normal_owner, "deferred fence order");
    std::puts("A6B PASS 1: real TCPIP enter -> fence admission -> exit marker -> fence; same core task");

    xEventGroupClearBits(events, completed);
    delayed_owner.expired = true; // Test policy input; no DNS completion is injected.
    delayed_owner.delayed = true;
    check(tcpip_try_callback(callback, &delayed_owner) == ERR_OK, "delayed admission");
    wait_for(waiting, "callback gate timeout");
    const EventBits_t early = xEventGroupWaitBits(events, completed, pdFALSE, pdTRUE,
                                                 pdMS_TO_TICKS(50));
    check((early & completed) == 0 && delayed_owner.retained, "fence must remain pending");
    check(xSemaphoreGive(release_gate) == pdTRUE, "release in-flight callback");
    wait_for(completed, "delayed fence timeout");
    verify_order(delayed_owner, "delayed fence order");
    check(delayed_owner.late_discarded, "expired policy marker discarded");
    std::puts("A6B PASS 2: caller wait expired while real callback blocked; retained owner, deferred fence after release");

    xEventGroupClearBits(events, completed | drained);
    check(tcpip_try_callback(saturation, &full_owner) == ERR_OK, "saturation admission");
    wait_for(drained, "queue drain timeout");
    check(full_owner.valid && full_owner.admission_failure == ERR_MEM &&
          full_owner.fence_result == ERR_MEM && full_owner.rejected_called == 0 &&
          full_owner.retained && full_owner.consumed == full_owner.queued &&
          full_owner.count == 2 && full_owner.order[0] == Event::saturated &&
          full_owner.order[1] == Event::drain, "ERR_MEM must retain owner and reject dispatch");
    std::printf("A6B PASS 3: actual admission/fence ERR_MEM; queued=%u mailbox=%u drained=%u owner retained\n",
                full_owner.queued, static_cast<unsigned>(TCPIP_MBOX_SIZE), full_owner.consumed);
    std::puts("A6B RUNTIME_CONFIRMED: 3/3 in-memory SDK cases; all owners static and unrecycled");
    std::puts("A6B NOT_RUN: DNS callback/registration clear, duplicate hostname, never-arriving DNS, cancellation, token recycle, TLS, close, hardware");
    std::puts("A6B UNPROVEN: DNS quiescence, caller-return bound, eventual reclamation, worker teardown");
    std::fflush(stdout);
    std::_Exit(0);
}
