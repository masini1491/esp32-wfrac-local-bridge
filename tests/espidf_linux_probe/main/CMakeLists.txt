idf_component_register(SRCS "callback_fence.cpp" REQUIRES lwip freertos)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=sys_mbox_post,--wrap=sys_arch_sem_wait")
