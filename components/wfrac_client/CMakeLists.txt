idf_component_register(
    SRCS "src/request_executor.cpp" "src/control_verification.cpp" "src/runtime_orchestrator.cpp"
    INCLUDE_DIRS "include"
    REQUIRES wfrac_api wfrac_device_model wfrac_transport
)
