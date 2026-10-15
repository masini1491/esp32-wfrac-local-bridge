// A7b isolated Linux SDK DNS handler tests. All UDP attempts are linker-wrapped.
// Static owners/captures are retained until process exit. No token reclamation.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"

#if !CONFIG_IDF_TARGET_LINUX || LWIP_TCPIP_CORE_LOCKING || !LWIP_IPV4 || !LWIP_DNS
#error "A7b requires the independently reported Linux serialized TCPIP/DNS/IPv4 configuration"
#endif

namespace {
constexpr unsigned owner_limit = 64, capture_limit = 256, packet_limit = 512;
constexpr EventBits_t ready = 1, registered = 2, finished = 4, waiting = 8;
constexpr TickType_t budget = pdMS_TO_TICKS(5000);
StaticEventGroup_t event_storage;
EventGroupHandle_t events;
StaticSemaphore_t gate_storage;
SemaphoreHandle_t gate;
TaskHandle_t core;
struct Capture {
    udp_pcb* pcb;
    udp_recv_fn recv;
    void* argument;
    ip_addr_t server;
    u16_t port, length;
    unsigned char bytes[packet_limit];
};
Capture captures[capture_limit]{};
unsigned wrapped = 0;
bool violation = false, smoke_null = false, recv_returned = false;
const char* mode;
ip_addr_t server, expected_address;
struct Owner {
    unsigned id = 0, calls = 0, fences = 0, queued = 0, drained = 0;
    bool started = false, retained = true, expired = false, discarded = false;
    bool delay = false, fail_fence = false, negative = false;
    err_t result = ERR_VAL, fence_result = ERR_VAL;
};
Owner owners[owner_limit];
struct Trace { const char* event; unsigned owner; };
Trace trace[256]{};
unsigned trace_size = 0, fence_count = 0, wanted_fences = 1;

[[noreturn]] void fail(const char* label) {
    std::printf("A7B FAIL/STOP: %s\n", label);
    std::fflush(stdout);
    std::_Exit(1);
}
void check(bool ok, const char* label) { if (!ok) fail(label); }
void record(const char* event, unsigned id = 0) {
    check(xTaskGetCurrentTaskHandle() == core, "not TCPIP task");
    check(trace_size < 256, "trace capacity");
    trace[trace_size++] = {event, id};
}
void wait_for(EventBits_t bit, const char* label) {
    check((xEventGroupWaitBits(events, bit, pdFALSE, pdTRUE, budget) & bit) == bit, label);
}
void post(tcpip_callback_fn fn, void* arg = nullptr) {
    check(tcpip_try_callback(fn, arg) == ERR_OK, "test dispatch admission");
}
void init_done(void*) {
    core = xTaskGetCurrentTaskHandle();
    xEventGroupSetBits(events, ready);
}
void set_server() {
    for (unsigned i = 0; i < DNS_MAX_SERVERS; ++i) dns_setserver(i, nullptr);
    dns_setserver(0, &server);
}
void fence(void* arg) {
    auto& owner = *static_cast<Owner*>(arg);
    record("deferred_fence", owner.id);
    check(recv_returned && owner.retained && owner.calls == 1, "fence observation");
    ++owner.fences;
    if (++fence_count == wanted_fences) xEventGroupSetBits(events, finished);
}
void filler(void* arg) {
    auto& owner = *static_cast<Owner*>(arg);
    if (++owner.drained == owner.queued) {
        record("queue_drained", owner.id);
        xEventGroupSetBits(events, finished);
    }
}
void found(const char*, const ip_addr_t* address, void* arg) {
    auto& owner = *static_cast<Owner*>(arg);
    record("DNS_found_enter", owner.id);
    check(owner.started && owner.retained && ++owner.calls == 1, "callback argument identity");
    check(owner.negative ? address == nullptr :
          address != nullptr && ip_addr_eq(address, &expected_address), "callback address");
    owner.discarded = owner.expired;
    if (owner.fail_fence) {
        for (unsigned i = 0; i <= TCPIP_MBOX_SIZE; ++i) {
            if (tcpip_try_callback(filler, &owner) != ERR_OK) break;
            ++owner.queued;
        }
    }
    owner.fence_result = tcpip_try_callback(fence, &owner);
    record(owner.fence_result == ERR_OK ? "fence_admitted" : "fence_rejected", owner.id);
    if (owner.fail_fence) check(owner.fence_result == ERR_MEM && owner.queued > 0, "fence ERR_MEM");
    else check(owner.fence_result == ERR_OK, "fence must enqueue");
    if (owner.delay) {
        xEventGroupSetBits(events, waiting);
        check(xSemaphoreTake(gate, budget) == pdTRUE, "delayed callback release timeout");
    }
    record("DNS_found_exit_marker", owner.id);
}
err_t lookup(const char* name, Owner& owner) {
    check(netif_list == nullptr && !violation && !owner.started, "pre-registration guard");
    owner.started = true;
    ip_addr_t answer{};
    owner.result = dns_gethostbyname_addrtype(name, &answer, found, &owner, LWIP_DNS_ADDRTYPE_IPV4);
    record(owner.result == ERR_INPROGRESS ? "registration_INPROGRESS" :
           owner.result == ERR_OK ? "registration_OK" : "registration_error", owner.id);
    return owner.result;
}
void smoke(void*) {
    check(netif_list == nullptr, "live netif before smoke");
    pbuf* packet = pbuf_alloc(PBUF_RAW, 4, PBUF_RAM);
    const unsigned char sentinel[4] = {0x71, 0x72, 0x73, 0x74};
    check(packet && pbuf_take(packet, sentinel, 4) == ERR_OK, "smoke pbuf");
    smoke_null = true;
    const err_t rc = udp_sendto(nullptr, packet, &server, 53);
    smoke_null = false;
    pbuf_free(packet);
    check(rc == ERR_OK && !violation && wrapped == 1 && captures[0].length == 4 &&
          std::memcmp(captures[0].bytes, sentinel, 4) == 0, "effective wrapper smoke");
    xEventGroupSetBits(events, finished);
}
void guard_negative(void*) {
    const err_t rc = udp_sendto(nullptr, nullptr, &server, 53);
    check(rc == ERR_ARG && violation && wrapped == 0, "malformed capture must fail closed");
    xEventGroupSetBits(events, finished);
}
void immediate(void*) {
    set_server();
    check(lookup("192.0.2.44", owners[0]) == ERR_OK && owners[0].calls == 0, "numeric ERR_OK");
    ip_addr_t answer{};
    check(dns_gethostbyname_addrtype("", &answer, found, &owners[1],
                                    LWIP_DNS_ADDRTYPE_IPV4) == ERR_ARG, "invalid ERR_ARG");
    for (unsigned i = 0; i < DNS_MAX_SERVERS; ++i) dns_setserver(i, nullptr);
    check(lookup("absent.a7b.test", owners[2]) == ERR_VAL, "absent server ERR_VAL");
    check(wrapped == 1 && !violation, "immediate paths must not attempt send");
    xEventGroupSetBits(events, finished);
}
void start(void*) {
    set_server();
    owners[0].delay = std::strcmp(mode, "late") == 0;
    owners[0].negative = std::strcmp(mode, "error") == 0;
    owners[0].fail_fence = std::strcmp(mode, "fence-error") == 0;
    check(lookup("owner.a7b.test", owners[0]) == ERR_INPROGRESS, "genuine registration");
    check(wrapped > 1 && !violation && captures[1].recv != nullptr, "original DNS send captured");
    if (std::strcmp(mode, "duplicate") == 0) {
        check((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0,
              "duplicate test requires reported coalescing configuration");
        wanted_fences = 2;
        check(lookup("owner.a7b.test", owners[1]) == ERR_INPROGRESS, "duplicate registration");
        check(wrapped == 2, "same hostname must coalesce in this configuration");
    }
    xEventGroupSetBits(events, registered);
}
void inject(void*) {
    auto& query = captures[1];
    check(query.length >= 16 && query.length <= packet_limit - 16 && query.pcb && query.recv,
          "bounded captured DNS query");
    owners[0].expired = std::strcmp(mode, "late") == 0;
    unsigned char reply[packet_limit]{};
    std::memcpy(reply, query.bytes, query.length);
    reply[2] = 0x81; reply[3] = owners[0].negative ? 0x83 : 0x80;
    reply[6] = 0; reply[7] = owners[0].negative ? 0 : 1;
    reply[8] = reply[9] = reply[10] = reply[11] = 0;
    unsigned length = query.length;
    if (!owners[0].negative) {
        const unsigned char answer[16] = {
            0xc0, 0x0c, 0, 1, 0, 1, 0, 0, 0, 60, 0, 4, 192, 0, 2, 44
        };
        std::memcpy(reply + length, answer, sizeof(answer)); length += sizeof(answer);
    }
    pbuf* packet = pbuf_alloc(PBUF_RAW, static_cast<u16_t>(length), PBUF_RAM);
    check(packet && pbuf_take(packet, reply, length) == ERR_OK, "response pbuf allocation");
    // recv consumes the pbuf and may free the PCB. Retire the capture BEFORE call.
    const udp_recv_fn receive = query.recv;
    udp_pcb* pcb = query.pcb;
    void* arg = query.argument;
    const ip_addr_t source = query.server;
    const u16_t port = query.port;
    query.pcb = nullptr; query.recv = nullptr;
    record("original_dns_recv_enter");
    receive(arg, pcb, packet, &source, port);
    // No PCB or pbuf access after ownership transfer.
    recv_returned = true;
    record("original_dns_recv_returned");
}
void cached(void*) {
    check(lookup("owner.a7b.test", owners[2]) == ERR_OK && owners[2].calls == 0 &&
          wrapped == 2, "cached ERR_OK cross-check, not private registration read");
    xEventGroupSetBits(events, registered);
}
void exhaust(void*) {
    set_server();
    check(DNS_TABLE_SIZE < owner_limit - 1, "test storage smaller than DNS table");
    unsigned pending = 0;
    bool exhausted = false;
    for (unsigned i = 0; i <= DNS_TABLE_SIZE; ++i) {
        char name[48];
        std::snprintf(name, sizeof(name), "retained-%u.a7b.test", i);
        const err_t rc = lookup(name, owners[i]);
        if (rc == ERR_MEM) { exhausted = true; break; }
        check(rc == ERR_INPROGRESS, "pending registration result");
        ++pending;
    }
    check(exhausted && pending > 0 && wrapped > 1 && !violation, "bounded DNS exhaustion");
    record("pending_owners_quarantined", pending);
    xEventGroupSetBits(events, registered);
}
void print_config() {
    std::printf("A7B_CONFIG LWIP_DNS_SECURE=%u DNS_TABLE_SIZE=%u CORE_LOCKING=%u "
                "TCPIP_MBOX_SIZE=%u LWIP_DNS=%u IPV4=%u IPV6=%u DEFAULT_ADDRTYPE=%u "
                "LWIP_HAVE_LOOPIF=%u LWIP_NETIF_LOOPBACK=%u\n",
        unsigned(LWIP_DNS_SECURE), unsigned(DNS_TABLE_SIZE), unsigned(LWIP_TCPIP_CORE_LOCKING),
        unsigned(TCPIP_MBOX_SIZE), unsigned(LWIP_DNS), unsigned(LWIP_IPV4), unsigned(LWIP_IPV6),
        unsigned(LWIP_DNS_ADDRTYPE_DEFAULT), unsigned(LWIP_HAVE_LOOPIF), unsigned(LWIP_NETIF_LOOPBACK));
}
}

extern "C" err_t __wrap_udp_sendto(udp_pcb* pcb, pbuf* packet,
                                   const ip_addr_t* destination, u16_t port) {
    // Sole outbound stub: bounded capture only. No forward, native output or function-pointer call.
    if (netif_list != nullptr || xTaskGetCurrentTaskHandle() != core || !packet ||
        !destination || (!pcb && !smoke_null) || packet->tot_len > packet_limit ||
        wrapped >= capture_limit || port != 53) {
        violation = true;
        return ERR_ARG;
    }
    Capture& capture = captures[wrapped++];
    capture.pcb = pcb;
    capture.recv = pcb ? pcb->recv : nullptr;
    capture.argument = pcb ? pcb->recv_arg : nullptr;
    capture.server = *destination;
    capture.port = port;
    capture.length = packet->tot_len;
    if (pbuf_copy_partial(packet, capture.bytes, capture.length, 0) != capture.length) {
        violation = true;
        return ERR_VAL;
    }
    return ERR_OK;
}

extern "C" void app_main() {
    mode = std::getenv("A7B_MODE");
    check(mode != nullptr, "explicit mode required; resolver NOT_RUN");
    if (std::strcmp(mode, "config") == 0) { print_config(); std::fflush(stdout); std::_Exit(0); }
    const bool is_smoke = std::strcmp(mode, "smoke") == 0;
    const bool is_guard = std::strcmp(mode, "guard") == 0;
    if (!is_smoke && !is_guard) {
        const char* proof = std::getenv("A7B_WRAP_PROVEN");
        check(proof && std::strcmp(proof, "verified-by-runner") == 0,
              "binary interception proof required; resolver NOT_RUN");
        check(std::strcmp(mode, "immediate") == 0 || std::strcmp(mode, "success") == 0 ||
              std::strcmp(mode, "error") == 0 || std::strcmp(mode, "duplicate") == 0 ||
              std::strcmp(mode, "late") == 0 || std::strcmp(mode, "fence-error") == 0 ||
              std::strcmp(mode, "exhaust") == 0, "unknown mode; resolver NOT_RUN");
    }
    for (unsigned i = 0; i < owner_limit; ++i) owners[i].id = i;
    check(ipaddr_aton("198.51.100.53", &server) && ipaddr_aton("192.0.2.44", &expected_address),
          "synthetic numeric addresses");
    events = xEventGroupCreateStatic(&event_storage);
    gate = xSemaphoreCreateBinaryStatic(&gate_storage);
    check(events && gate, "synchronization allocation");
    tcpip_init(init_done, nullptr);
    wait_for(ready, "TCPIP initialization timeout");
    check(core != xTaskGetCurrentTaskHandle(), "real TCPIP task identity");
    post(is_guard ? guard_negative : smoke);
    wait_for(finished, "wrapper smoke timeout");
    if (is_smoke || is_guard) {
        std::printf("A7B PREEXEC %s PASS wrapped=%u violation=%u DNS_registration=NOT_RUN\n",
                    mode, wrapped, unsigned(violation));
        std::fflush(stdout); std::_Exit(0);
    }
    xEventGroupClearBits(events, finished);
    if (std::strcmp(mode, "immediate") == 0) {
        post(immediate); wait_for(finished, "immediate cases timeout");
    } else if (std::strcmp(mode, "exhaust") == 0) {
        post(exhaust); wait_for(registered, "exhaustion test timeout");
        check((xEventGroupWaitBits(events, finished, pdFALSE, pdTRUE, pdMS_TO_TICKS(50)) & finished) == 0,
              "unexpected pending completion");
        for (const auto& owner : owners) check(owner.retained && owner.calls == 0, "pending retention");
    } else {
        post(start); wait_for(registered, "registration timeout");
        if (std::strcmp(mode, "late") == 0)
            check((xEventGroupWaitBits(events, finished, pdFALSE, pdTRUE, pdMS_TO_TICKS(50)) & finished) == 0,
                  "unexpected completion before in-memory reply");
        post(inject);
        if (std::strcmp(mode, "late") == 0) {
            wait_for(waiting, "delayed DNS callback timeout");
            check((xEventGroupWaitBits(events, finished, pdFALSE, pdTRUE, pdMS_TO_TICKS(50)) & finished) == 0,
                  "fence executed inside DNS callback");
            check(owners[0].retained && xSemaphoreGive(gate) == pdTRUE, "retained callback release");
        }
        wait_for(finished, "completion/fence timeout");
        check(recv_returned && owners[0].calls == 1 && owners[0].retained, "DNS handler observation");
        if (owners[0].fail_fence) {
            check(owners[0].fences == 0 && owners[0].fence_result == ERR_MEM &&
                  owners[0].drained == owners[0].queued, "failed fence owner quarantine");
        } else check(owners[0].fences == 1, "deferred DNS fence");
        if (std::strcmp(mode, "late") == 0) check(owners[0].discarded, "late address discarded");
        if (std::strcmp(mode, "duplicate") == 0)
            check(owners[1].calls == 1 && owners[1].fences == 1 && owners[1].retained,
                  "distinct duplicate argument");
        if (std::strcmp(mode, "success") == 0) {
            xEventGroupClearBits(events, registered); post(cached); wait_for(registered, "cache check timeout");
        }
    }
    check(!violation && netif_list == nullptr, "zero-egress capture guard");
    for (unsigned i = 0; i < trace_size; ++i)
        std::printf("A7B_EVENT %u %s owner=%u\n", i, trace[i].event, trace[i].owner);
    std::printf("A7B SDK_DNS_HANDLER_RUNTIME %s PASS wrapped=%u retained=YES recycled=NO\n", mode, wrapped);
    std::puts("A7B UNPROVEN private found=NULL runtime read; cancellation; eventual reclaim; teardown; caller-return/close bounds");
    std::fflush(stdout);
    std::_Exit(0);
}
