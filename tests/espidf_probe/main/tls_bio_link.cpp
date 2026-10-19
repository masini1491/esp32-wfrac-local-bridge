// D2-P1-T COMPILE/LINK ONLY. app_main keeps our address but never calls us.
// No handshake, socket/connect, certificate loading, keys, network or execution.
#include <cstddef>
#include <cstdint>
#include "sdkconfig.h"
#include "mbedtls/build_info.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/net_sockets.h"

#if !CONFIG_IDF_TARGET_ESP32C3 && !CONFIG_IDF_TARGET_ESP32C6
#error "D2-P1-T only establishes the admitted C3/C6 build contract"
#endif
#if !CONFIG_LWIP_ENABLE
#error "D2-P1-T requires the actual lwIP-backed ESP-IDF port"
#endif
#ifdef MBEDTLS_NET_C
#error "Expected ESP-IDF port path, not upstream MBEDTLS_NET_C implementation"
#endif

namespace {
template<class A, class B> struct Same { static constexpr bool value = false; };
template<class A> struct Same<A,A> { static constexpr bool value = true; };
// Header signature checks ONLY. No address/call/reference emitted for these
// upstream prototypes: their target implementations must NOT be assumed.
static_assert(Same<decltype(&mbedtls_net_poll),
                   int(*)(mbedtls_net_context*,std::uint32_t,std::uint32_t)>::value);
static_assert(Same<decltype(&mbedtls_net_close),void(*)(mbedtls_net_context*)>::value);
struct TlsOwner {
    mbedtls_ssl_context ssl{};
    mbedtls_ssl_config config{};
    mbedtls_x509_crt ca{}; // Empty test-owned chain; no certificate/private data.
    mbedtls_net_context bio{};
    char empty_hostname[1]{};
    int result = 0;
};
TlsOwner owner;
}
extern "C" void wfrac_d2_tls_link_anchor() {
    mbedtls_ssl_init(&owner.ssl);
    mbedtls_ssl_config_init(&owner.config);
    mbedtls_x509_crt_init(&owner.ca);
    mbedtls_net_init(&owner.bio);
    owner.result = mbedtls_ssl_config_defaults(&owner.config, MBEDTLS_SSL_IS_CLIENT,
                                              MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (owner.result == 0) {
        mbedtls_ssl_conf_authmode(&owner.config, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(&owner.config, &owner.ca, nullptr);
        owner.result = mbedtls_ssl_setup(&owner.ssl, &owner.config);
        if (owner.result == 0) {
            owner.result = mbedtls_ssl_set_hostname(&owner.ssl, owner.empty_hostname);
            mbedtls_ssl_set_bio(&owner.ssl, &owner.bio, mbedtls_net_send, mbedtls_net_recv, nullptr);
        }
    }
    int (*volatile timeout_symbol)(void*,unsigned char*,std::size_t,std::uint32_t) = mbedtls_net_recv_timeout;
    void (*volatile free_symbol)(mbedtls_net_context*) = mbedtls_net_free;
    (void)timeout_symbol; (void)free_symbol; // Link references, not I/O or close calls.
    // Borrowed config/CA/BIO remain allocated past SSL free. No socket-close
    // acknowledgment is implied. This whole graph is NEVER EXECUTED.
    mbedtls_ssl_free(&owner.ssl);
    mbedtls_ssl_config_free(&owner.config);
    mbedtls_x509_crt_free(&owner.ca);
}
