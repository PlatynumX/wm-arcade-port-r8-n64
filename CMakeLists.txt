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
    src/core/select_continue.c
    src/core/award.c
    src/core/pregame.c
    src/core/anim.c
    src/core/game.c
    src/core/ropes.c
    src/core/demo.c
    src/core/match.c
    src/core/movement.c
    src/core/bret_backend.c
    src/core/wrestler_backend.c
    src/core/human_input.c
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
    src/generated/bret_defense.c
    src/generated/bret_frame_geometry.c
    src/generated/progress_wrestlers.c
)
# BEGIN FIX38 CUMULATIVE ARCADE SOURCE PORTS
target_sources(wmcore PRIVATE
    src/core/arcade/wm_arcade_anim_combat.c
    src/core/arcade/wm_arcade_attach_anim.c
    src/core/arcade/wm_arcade_bam.c
    src/core/arcade/wm_arcade_bret.c
    src/core/arcade/wm_arcade_bret_tables.c
    src/core/arcade/wm_arcade_closest.c
    src/core/arcade/wm_arcade_combat.c
    src/core/arcade/wm_arcade_confine.c
    src/core/arcade/wm_arcade_doink.c
    src/core/arcade/wm_arcade_drone.c
    src/core/arcade/wm_arcade_drone_data.c
    src/core/arcade/wm_arcade_joystat.c
    src/core/arcade/wm_arcade_lex.c
    src/core/arcade/wm_arcade_lifebar.c
    src/core/arcade/wm_arcade_mode_dead.c
    src/core/arcade/wm_arcade_move_dispatch.c
    src/core/arcade/wm_arcade_razor.c
    src/core/arcade/wm_arcade_razor_tables.c
    src/core/arcade/wm_arcade_react.c
    src/core/arcade/wm_arcade_react1_core.c
    src/core/arcade/wm_arcade_react2_core.c
    src/core/arcade/wm_arcade_react3_core.c
    src/core/arcade/wm_arcade_react4_core.c
    src/core/arcade/wm_arcade_react5_core.c
    src/core/arcade/wm_arcade_react6_core.c
    src/core/arcade/wm_arcade_react7_core.c
    src/core/arcade/wm_arcade_react8_core.c
    src/core/arcade/wm_arcade_react9_core.c
    src/core/arcade/wm_arcade_roster.c
    src/core/arcade/wm_arcade_round.c
    src/core/arcade/wm_arcade_shawn.c
    src/core/arcade/wm_arcade_special.c
    src/core/arcade/wm_arcade_taker.c
    src/core/arcade/wm_arcade_wrestler_port.c
    src/core/arcade/wm_arcade_yoko.c
    src/core/arcade/wmania_attract_adapter.c
    src/core/arcade/wmania_attract_core.c
    src/core/arcade/wmania_attract_data.c
    src/core/arcade/wmania_attract_operator.c
    src/core/arcade/wmania_attract_secret.c
    src/core/arcade/wmania_attract_time.c
    src/core/arcade/wmania_attract_visuals.c
    src/core/arcade/wmania_hiscore_adapter.c
    src/core/arcade/wmania_hiscore_core.c
    src/core/arcade/wmania_hiscore_counter.c
    src/core/arcade/wmania_hiscore_entry.c
    src/core/arcade/wmania_hiscore_factory.c
    src/core/arcade/wmania_hiscore_persist.c
    src/core/arcade/wmania_hiscore_present.c
    src/core/arcade/wmania_hiscore_special.c
    src/core/arcade/wmania_hiscore_system.c
    src/core/arcade/wmania_ring_climb.c
    src/core/arcade/wmania_ring_geometry.c
    src/core/arcade/wmania_ring_out.c
    src/core/arcade/wmania_rng.c
    src/core/arcade/wmania_rope_command.c
    src/core/arcade/wmania_rope_runtime.c
    src/core/arcade/wmania_rope_source_data.c
    src/core/arcade/wmania_rope_spawn.c
)
# END FIX38 CUMULATIVE ARCADE SOURCE PORTS


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

    # Fix38 combat/ring/RNG/hiscore host verification tests. These were staged
    # under tests/arcade_port/fix38 alongside the src/core/arcade sources they
    # exercise but were never registered with CTest; the src/core/arcade code
    # itself has been buildable and linked into wmcore/wm_headless since fix38.
    file(GLOB WM_ARCADE_STAGE25_TESTS
         ${CMAKE_SOURCE_DIR}/tests/arcade_port/fix38/combat_stage25/test_combat_stage*.c)
    file(GLOB WM_ARCADE_RING_RNG_TESTS
         ${CMAKE_SOURCE_DIR}/tests/arcade_port/fix38/ring_rng/test_*.c)

    foreach(test_src ${WM_ARCADE_STAGE25_TESTS} ${WM_ARCADE_RING_RNG_TESTS})
        get_filename_component(test_name ${test_src} NAME_WE)
        set(test_target wm_${test_name})
        add_executable(${test_target} ${test_src})
        target_link_libraries(${test_target} PRIVATE wmcore)
        # These tests were staged as standalone units against the arcade/
        # headers directly (e.g. "wm_arcade_drone.h"), not the wm/arcade/
        # public include path; add it just for these targets.
        target_include_directories(${test_target} PRIVATE include/wm/arcade)
        target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic)
        # These tests verify themselves with plain assert(), which the
        # project's documented release build (CMAKE_BUILD_TYPE=Release)
        # would otherwise silently strip via the -DNDEBUG in
        # CMAKE_C_FLAGS_RELEASE, making every assertion here a no-op and
        # every one of these tests "pass" regardless of what it checks.
        # -UNDEBUG (appended after the build type's flags, so it wins)
        # keeps assert() live in these binaries no matter the build type.
        target_compile_options(${test_target} PRIVATE -UNDEBUG)
        add_test(NAME ${test_target} COMMAND ${test_target})
    endforeach()
endif()
