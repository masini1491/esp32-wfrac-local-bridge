idf_component_register(
    SRCS "src/protocol_mapping.cpp" "src/device_state.cpp" "src/protocol_command_mapping.cpp" "src/protocol_request_mapping.cpp"
    INCLUDE_DIRS "include"
    REQUIRES wfrac_protocol
)
