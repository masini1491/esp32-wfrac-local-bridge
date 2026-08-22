#pragma once

#include "wfrac_transport/transport.hpp"

namespace wfrac_transport {

enum class SchemeKnowledge {
  unknown,
  http_confirmed,
  https_confirmed,
};

struct SchemeCache {
  SchemeKnowledge knowledge = SchemeKnowledge::unknown;
};

enum class SchemeNextAction {
  stop,
  retry_http_compatibility,
};

enum class SchemePolicyError {
  none,
  invalid_argument,
};

struct SchemeAttemptDecision {
  SchemePolicyError error = SchemePolicyError::none;
  SchemeNextAction next_action = SchemeNextAction::stop;
  bool scheme_confirmed = false;
};

TransportScheme select_scheme(const SchemeCache& cache);

SchemeAttemptDecision observe_scheme_attempt(SchemeCache* cache, TransportScheme attempted_scheme,
                                             TransportError result);

SchemePolicyError clear_scheme_cache(SchemeCache* cache);

}  // namespace wfrac_transport
