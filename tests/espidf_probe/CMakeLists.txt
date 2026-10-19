cmake_minimum_required(VERSION 3.16)
set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wfrac_a5_link_only)
target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE "-Wl,--cref")
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
    COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/verify_tls_link.py"
            "${CMAKE_BINARY_DIR}" --target "${IDF_TARGET}"
    VERBATIM)
