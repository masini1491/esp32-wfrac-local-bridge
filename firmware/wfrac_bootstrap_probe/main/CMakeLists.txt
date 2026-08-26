idf_component_register(
    SRCS "app_main.cpp" "bootstrap_probe.cpp"
    INCLUDE_DIRS "."
    REQUIRES wfrac_api wfrac_client wfrac_transport wfrac_transport_espidf wfrac_time_espidf esp_event esp_netif esp_wifi
)
