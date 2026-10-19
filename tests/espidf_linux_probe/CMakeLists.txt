cmake_minimum_required(VERSION 3.16)
set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wfrac_a6b_linux)
idf_build_get_property(project_elf EXECUTABLE)
add_custom_command(TARGET ${project_elf} POST_BUILD
    COMMAND ${PYTHON} -B "${CMAKE_CURRENT_SOURCE_DIR}/verify_close_t2a.py"
    COMMENT "CLOSE-T2A synthetic verifier guards (no SDK execution)")
