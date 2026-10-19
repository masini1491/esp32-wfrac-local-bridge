idf_component_register(SRCS "loopback_link.cpp" REQUIRES lwip)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
