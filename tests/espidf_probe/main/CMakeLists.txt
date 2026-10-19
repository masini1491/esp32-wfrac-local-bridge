idf_component_register(SRCS "callback_fence_link.cpp" "tls_bio_link.cpp"
                       INCLUDE_DIRS "." REQUIRES lwip mbedtls)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
