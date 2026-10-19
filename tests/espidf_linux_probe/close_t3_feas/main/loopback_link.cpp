// CLOSE-T3-FEAS: compile/link anchor ONLY. Never execute this fixture ELF.
#include "sdkconfig.h"
#include "lwip/opt.h"
#include "lwip/netif.h"

#if !CONFIG_IDF_TARGET_LINUX || !CONFIG_LWIP_ENABLE
#error "CLOSE-T3-FEAS requires the isolated Linux lwIP build"
#endif
#if !CONFIG_LWIP_NETIF_LOOPBACK || !LWIP_HAVE_LOOPIF || !LWIP_NETIF_LOOPBACK
#error "CLOSE-T3-FEAS requires the actual enabled SDK loopback mapping"
#endif
#if !LWIP_IPV4
#error "CLOSE-T3-FEAS requires the separately reported IPv4 configuration"
#endif

extern "C" void app_main() {
    // Volatile address references retain real SDK code and linker consumers.
    // No function is called: no tcpip_init, netif startup, packet or socket.
    decltype(&netif_init) volatile init_reference = &netif_init;
    decltype(&netif_loop_output) volatile output_reference = &netif_loop_output;
    decltype(&netif_poll) volatile poll_reference = &netif_poll;
    (void)init_reference;
    (void)output_reference;
    (void)poll_reference;
}
