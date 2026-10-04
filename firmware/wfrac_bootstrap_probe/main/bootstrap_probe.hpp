#pragma once

#include "diagnostic_form.hpp"

// Synchronous firmware-local executor. reuse_ap requires initialized diagnostic
// AP/STA infrastructure; it returns to AP-only mode without destroying the UI.
diagnostic::Result execute_diagnostic_probe(const diagnostic::Input& input, bool reuse_ap);
void start_diagnostic_webui();
void run_wfrac_bootstrap_probe();
