// A5 compile/link fixture ONLY. No callback, DNS or network execution in app_main.
// This is not an admitted resolver, cancellation API or reclamation algorithm.
#include "lwip/dns.h"
#include "lwip/tcpip.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/opt.h"

namespace {
enum class Phase { retained, queued, registered, fence_pending, fence_observed, rejected };

// Static retained storage: no caller buffer, stack token, release or recycle path.
// Fields below are meaningful only in the TCPIP core context; no cross-thread reader.
struct Owner {
    Phase phase = Phase::retained;
    bool expired = false;
    bool late_result_discarded = false;
    bool has_address = false;
    bool queue_full = false;
    bool fence_failed = false;
    ip_addr_t address{};
    err_t registration_result = ERR_INPROGRESS;
    err_t fence_result = ERR_INPROGRESS;
};

void fence(void* context) {
    auto& owner = *static_cast<Owner*>(context);
    // Seeing this event does NOT prove safe reclamation in this LINK_ONLY fixture.
    owner.phase = Phase::fence_observed;
}

void found(const char*, const ip_addr_t* address, void* context) {
    auto& owner = *static_cast<Owner*>(context);
    if (owner.expired) {
        owner.late_result_discarded = true;
        owner.has_address = false;
    } else if (address != nullptr) {
        owner.address = *address;
        owner.has_address = true;
    }
    owner.phase = Phase::fence_pending;
    owner.fence_result = tcpip_try_callback(fence, &owner);
    if (owner.fence_result != ERR_OK) {
        owner.queue_full = owner.fence_result == ERR_MEM;
        owner.fence_failed = true;
        owner.phase = Phase::retained;
    }
}

void registration(void* context) {
    auto& owner = *static_cast<Owner*>(context);
    // This entry is linked, NEVER dispatched by this fixture. Calling it would
    // require independent runtime authorization and a real core-context proof.
    owner.registration_result = dns_gethostbyname_addrtype(
        "a5-offline-probe.invalid", &owner.address, found, &owner, LWIP_DNS_ADDRTYPE_DEFAULT);
    if (owner.registration_result == ERR_INPROGRESS) {
        owner.phase = Phase::registered;
    } else if (owner.registration_result == ERR_OK) {
        owner.has_address = !owner.expired;
        owner.phase = Phase::retained;
    } else {
        owner.queue_full = owner.registration_result == ERR_MEM;
        owner.phase = Phase::rejected;
    }
}

err_t admission(Owner& owner) {
    owner.phase = Phase::queued;
    const err_t result = tcpip_try_callback(registration, &owner);
    if (result != ERR_OK) {
        owner.queue_full = result == ERR_MEM;
        owner.phase = Phase::rejected;
    }
    return result;
}

Owner retained_owner;
}  // namespace

extern "C" void wfrac_d2_tls_link_anchor();
extern "C" void app_main() {
    // Keep the complete reference graph reachable to the linker without invoking
    // any SDK operation. Do not run/flash this fixture as lifecycle evidence.
    err_t (*volatile linked_entry)(Owner&) = admission;
    Owner* volatile linked_storage = &retained_owner;
    (void)linked_entry;
    (void)linked_storage;
    void (*volatile tls_link_only)() = wfrac_d2_tls_link_anchor;
    (void)tls_link_only; // Retain symbol graph only; NEVER invoke this function.
}
