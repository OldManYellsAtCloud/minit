cmake_minimum_required(VERSION 3.16)

project(minit LANGUAGES C)
set(CMAKE_C_STANDARD 23)

add_executable(minit main.c
    config_file.h
    config_file.c
    text_utils.h
    text_utils.c
    task.h
    task.c
    reaper.h
    reaper.c
    native_task.h)

add_executable(minit_test_text_utils
    ctest.h
    test_text_utils.c
    text_utils.c
    text_utils.h)

add_executable(minit_test_config_file
    ctest.h
    test_config_file.c
    config_file.c
    config_file.h
    text_utils.c
    text_utils.h)

add_executable(minit_test_task
    ctest.h
    test_task.c
    config_file.c
    config_file.h
    text_utils.c
    text_utils.h)

add_library(sample_native_task SHARED
    sample_native_task.c
    native_task.h)

set_target_properties(sample_native_task PROPERTIES PREFIX ""
                                                    SUFFIX "")

include(GNUInstallDirs)
install(TARGETS minit
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_compile_definitions(minit PUBLIC -DDEBUG_BUILD -DBENCHMARK)

add_custom_command(TARGET minit_test_config_file POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/test_assets/testconf.cfg
        $<TARGET_FILE_DIR:minit_test_config_file>
)
