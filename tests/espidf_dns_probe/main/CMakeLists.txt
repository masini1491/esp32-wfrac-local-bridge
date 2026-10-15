idf_component_register(SRCS "dns_lifecycle.cpp" REQUIRES lwip freertos)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=udp_sendto")
