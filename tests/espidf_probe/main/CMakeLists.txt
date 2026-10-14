idf_component_register(SRCS "callback_fence_link.cpp" INCLUDE_DIRS "." REQUIRES lwip)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
