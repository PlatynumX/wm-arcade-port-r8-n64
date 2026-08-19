cmake_minimum_required(VERSION 3.16)
project(wm_arcade_port C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(WM_BUILD_TESTS "Build host-side verification tests" ON)

add_library(wmcore STATIC
    src/core/process.c
    src/core/source_clock.c
    src/core/bmod.c
    src/core/select.c
    src/core/select_screen.c
    src/core/anim.c
    src/core/game.c
    src/core/ropes.c
    src/core/demo.c
    src/core/visual.c
    src/core/roster.c
    src/core/attract.c
    src/core/audio.c
    src/core/app.c
    src/core/composite.c
    src/generated/attract_sequence.c
    src/generated/select_tables.c
    src/generated/port_status.c
    src/generated/finish_sequences.c
    src/generated/bret_visuals.c
    src/generated/bret_attacks.c
)
target_include_directories(wmcore PUBLIC include)
target_compile_options(wmcore PRIVATE -Wall -Wextra -Wpedantic)

add_executable(wm_headless src/platform/headless/main.c)
target_link_libraries(wm_headless PRIVATE wmcore)
target_compile_options(wm_headless PRIVATE -Wall -Wextra -Wpedantic)

if(WM_BUILD_TESTS)
    enable_testing()
    add_executable(wm_tests tests/test_core.c)
    target_link_libraries(wm_tests PRIVATE wmcore)
    target_compile_options(wm_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME wm_core COMMAND wm_tests)

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME wm_asmseq
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_asmseq.py)
    add_test(NAME wm_source_tools
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_source_tools.py)
endif()
