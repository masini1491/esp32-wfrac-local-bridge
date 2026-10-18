// CLOSE-F1: deliberately synthetic control-flow stand-ins, NOT SDK execution.
// Sources: ESP-IDF 7101770dc6db2667b3c477cc31365dd1acd6db4e:
// esp_http_client.c cleanup L1101-1142 / perform L1595-1619;
// tcp_transport/transport.c L72-117; transport_ssl.c L332-360;
// esp-tls/esp_tls.c L180-198. Scope: discarded status and lost ownership only.
// No SDK headers, socket, TLS, HTTP parser, network, threads or real allocation.
#include <cstdio>
#include <cstdlib>

namespace {
constexpr int ok = 0;
constexpr int failed = -1;
enum class Disposition { owned, released, unknown };
struct Transport {
    int handle = 7; // Fictional handle, never an OS descriptor.
    int injected_release_result = ok;
    int injected_destroy_result = ok;
    unsigned attempts = 0;
    bool tls_context = false;
    bool http_active = true;
    bool destroyed = false;
    Disposition disposition = Disposition::owned;
};
void check(bool value, const char* label) {
    if (!value) {
        std::printf("F1 FAIL: %s\n", label);
        std::exit(1);
    }
}
int synthetic_release(Transport& t) {
    ++t.attempts;
    // A failed release does NOT establish whether a real resource was freed.
    t.disposition = t.injected_release_result == ok ? Disposition::released
                                                  : Disposition::unknown;
    return t.injected_release_result;
}
int base_close(Transport& t) {
    int result = failed;
    if (t.tls_context || t.handle >= 0) {
        result = synthetic_release(t);
        // Models destroy freeing context / base_close invalidating sockfd even
        // on failure. No native TLS context or resource is actually destroyed.
        t.tls_context = false;
        t.handle = -1;
    }
    return result;
}
int http_close(Transport& t) {
    if (!t.http_active) return ok;
    t.http_active = false; // HTTP resets state before returning transport status.
    return base_close(t);
}
int base_destroy(Transport& t) {
    (void)base_close(t);
    t.destroyed = true;
    return ok;
}
int destroy_callback(Transport& t) {
    (void)base_destroy(t);
    // Only this case supplies a synthetic alternative _destroy callback status.
    // The pinned base_destroy normally returns 0 even after failed base_close.
    return t.injected_destroy_result;
}
int transport_destroy(Transport& t) {
    (void)destroy_callback(t); // Upstream discards its _destroy callback status.
    return ok;
}
int list_destroy(Transport& t) {
    (void)transport_destroy(t);
    return ok;
}
int http_cleanup(Transport& t) {
    (void)http_close(t);
    (void)list_destroy(t);
    return ok;
}
int perform_finish(Transport& t, int response_result, bool keep_alive = false) {
    if (!keep_alive) (void)http_close(t);
    return response_result;
}
bool lost_unknown(const Transport& t) {
    return t.handle == -1 && t.disposition == Disposition::unknown;
}
}
int main() {
    Transport direct;
    direct.injected_release_result = failed;
    check(http_close(direct) == failed && lost_unknown(direct), "direct failure visible, handle lost");
    std::puts("F1 SYNTHETIC_PASS 1: direct close reports injected failure; invalid handle is not release proof");

    Transport completed;
    completed.injected_release_result = failed;
    check(perform_finish(completed, ok) == ok && completed.attempts == 1 &&
          lost_unknown(completed), "perform can mask close failure");
    std::puts("F1 SYNTHETIC_PASS 2: successful response status masks failed close");
    check(http_cleanup(completed) == ok && completed.attempts == 1 &&
          completed.destroyed && lost_unknown(completed), "cleanup cannot recover lost handle");
    std::puts("F1 SYNTHETIC_PASS 3: cleanup returns success without another lower release after handle loss");

    Transport cleanup_only;
    cleanup_only.injected_release_result = failed;
    check(http_cleanup(cleanup_only) == ok && cleanup_only.attempts == 1 &&
          lost_unknown(cleanup_only), "cleanup drops close result");
    std::puts("F1 SYNTHETIC_PASS 4: cleanup-only release failure is discarded");

    Transport list;
    list.injected_destroy_result = failed;
    check(list_destroy(list) == ok && list.destroyed, "destroy/list drop destroy status");
    std::puts("F1 SYNTHETIC_PASS 5: list success does not certify per-transport destroy success");

    Transport prior_error;
    prior_error.injected_release_result = failed;
    check(perform_finish(prior_error, -23) == -23 && lost_unknown(prior_error), "original error retained");
    std::puts("F1 SYNTHETIC_PASS 6: original response error retained, not converted to success");

    Transport tls;
    tls.tls_context = true;
    tls.injected_release_result = failed;
    check(http_close(tls) == failed && !tls.tls_context && lost_unknown(tls), "synthetic TLS owner loss");
    std::puts("F1 SYNTHETIC_PASS 7: synthetic context release cannot certify lower socket disposition");

    Transport positive;
    check(perform_finish(positive, ok) == ok && http_cleanup(positive) == ok &&
          positive.attempts == 1 && positive.disposition == Disposition::released,
          "positive release control");
    Transport persistent;
    check(perform_finish(persistent, ok, true) == ok && persistent.attempts == 0 &&
          persistent.handle >= 0, "keep-alive control");
    std::puts("F1 SYNTHETIC_PASS 8: successful-release and keep-alive controls");
    std::puts("F1 NOT_RUN: actual SDK HTTP/TLS/socket/close ERR_MEM; real leak, worker safety and cleanup timing UNPROVEN");
}
