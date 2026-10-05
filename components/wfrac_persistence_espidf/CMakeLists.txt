idf_component_register(
    SRCS "src/nvs_persistence.cpp"
    INCLUDE_DIRS "include"
    REQUIRES wfrac_persistence
    PRIV_REQUIRES nvs_flash
)
