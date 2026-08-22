idf_component_register(
    SRCS "src/protocol_mapping.cpp" "src/device_state.cpp"
    INCLUDE_DIRS "include"
    REQUIRES wfrac_protocol
)
