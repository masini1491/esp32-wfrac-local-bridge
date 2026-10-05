idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES wfrac_api wfrac_client wfrac_device_model wfrac_protocol wfrac_transport wfrac_transport_espidf wfrac_time_espidf wfrac_matter_adapter wfrac_persistence_espidf
)
