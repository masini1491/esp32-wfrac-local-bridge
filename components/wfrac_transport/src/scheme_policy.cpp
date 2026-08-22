#include "wfrac_transport/scheme_policy.hpp"

namespace wfrac_transport {

TransportScheme select_scheme(const SchemeCache& cache) {
  return cache.knowledge == SchemeKnowledge::http_confirmed ? TransportScheme::http : TransportScheme::https;
}

SchemeAttemptDecision observe_scheme_attempt(SchemeCache* cache, TransportScheme attempted_scheme,
                                             TransportError result) {
  if (cache == nullptr) {
    return {SchemePolicyError::invalid_argument, SchemeNextAction::stop, false};
  }

  if (result == TransportError::none || result == TransportError::response_too_large) {
    cache->knowledge = attempted_scheme == TransportScheme::https ? SchemeKnowledge::https_confirmed
                                                                  : SchemeKnowledge::http_confirmed;
    return {SchemePolicyError::none, SchemeNextAction::stop, true};
  }

  if (cache->knowledge == SchemeKnowledge::unknown && attempted_scheme == TransportScheme::https &&
      result == TransportError::scheme_mismatch) {
    return {SchemePolicyError::none, SchemeNextAction::retry_http_compatibility, false};
  }

  return {SchemePolicyError::none, SchemeNextAction::stop, false};
}

SchemePolicyError clear_scheme_cache(SchemeCache* cache) {
  if (cache == nullptr) return SchemePolicyError::invalid_argument;
  cache->knowledge = SchemeKnowledge::unknown;
  return SchemePolicyError::none;
}

}  // namespace wfrac_transport
