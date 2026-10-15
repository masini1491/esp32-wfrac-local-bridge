idf_component_register(SRCS "callback_fence.cpp" REQUIRES lwip freertos)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
