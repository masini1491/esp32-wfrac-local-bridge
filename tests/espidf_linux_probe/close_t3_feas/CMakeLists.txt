cmake_minimum_required(VERSION 3.16)
set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wfrac_t3_feas_link_only)
idf_build_get_property(project_elf EXECUTABLE)
target_link_options(${project_elf} PRIVATE "-Wl,--cref")
add_custom_command(TARGET ${project_elf} POST_BUILD
    COMMAND ${PYTHON} -B "${CMAKE_CURRENT_SOURCE_DIR}/verify_build.py" "${CMAKE_BINARY_DIR}"
    COMMENT "CLOSE-T3-FEAS static build/link proof; NEVER executes fixture ELF")
