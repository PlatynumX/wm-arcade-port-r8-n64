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
    src/generated/progress_wrestlers.c
# BEGIN FIX39 SOURCE-DIRECT MERGE
    src/fix39/wm_arcade_anim_combat.c
    src/fix39/wm_arcade_attach_anim.c
    src/fix39/wm_arcade_bam.c
    src/fix39/wm_arcade_bret.c
    src/fix39/wm_arcade_bret_tables.c
    src/fix39/wm_arcade_combat.c
    src/fix39/wm_arcade_doink.c
    src/fix39/wm_arcade_drone.c
    src/fix39/wm_arcade_drone_source_tables.c
    src/fix39/wm_arcade_fireworks.c
    src/fix39/wm_arcade_lex.c
    src/fix39/wm_arcade_matchflow.c
    src/fix39/wm_arcade_move_dispatch.c
    src/fix39/wm_arcade_movement.c
    src/fix39/wm_arcade_razor.c
    src/fix39/wm_arcade_razor_tables.c
    src/fix39/wm_arcade_react.c
    src/fix39/wm_arcade_react1_core.c
    src/fix39/wm_arcade_react2_core.c
    src/fix39/wm_arcade_react3_core.c
    src/fix39/wm_arcade_react4_core.c
    src/fix39/wm_arcade_react5_core.c
    src/fix39/wm_arcade_react6_core.c
    src/fix39/wm_arcade_react7_core.c
    src/fix39/wm_arcade_react8_core.c
    src/fix39/wm_arcade_react9_core.c
    src/fix39/wm_arcade_roster.c
    src/fix39/wm_arcade_shawn.c
    src/fix39/wm_arcade_special.c
    src/fix39/wm_arcade_story.c
    src/fix39/wm_arcade_taker.c
    src/fix39/wm_arcade_wrestler_port.c
    src/fix39/wm_arcade_yoko.c
    src/fix39/wm_fix39_runtime.c
    src/fix39/wmania_attract_adapter.c
    src/fix39/wmania_attract_core.c
    src/fix39/wmania_attract_data.c
    src/fix39/wmania_attract_live.c
    src/fix39/wmania_attract_operator.c
    src/fix39/wmania_attract_secret.c
    src/fix39/wmania_attract_time.c
    src/fix39/wmania_attract_visuals.c
    src/fix39/wmania_hiscore_adapter.c
    src/fix39/wmania_hiscore_core.c
    src/fix39/wmania_hiscore_counter.c
    src/fix39/wmania_hiscore_entry.c
    src/fix39/wmania_hiscore_factory.c
    src/fix39/wmania_hiscore_persist.c
    src/fix39/wmania_hiscore_present.c
    src/fix39/wmania_hiscore_special.c
    src/fix39/wmania_hiscore_system.c
    src/fix39/wmania_ring_climb.c
    src/fix39/wmania_ring_geometry.c
    src/fix39/wmania_ring_onscreen.c
    src/fix39/wmania_ring_out.c
    src/fix39/wmania_rng.c
    src/fix39/wmania_rope_command.c
    src/fix39/wmania_rope_runtime.c
    src/fix39/wmania_rope_source_data.c
    src/fix39/wmania_rope_spawn.c
# END FIX39 SOURCE-DIRECT MERGE
)
# BEGIN FIX38 CUMULATIVE ARCADE SOURCE PORTS
target_sources(wmcore PRIVATE
    src/core/arcade/wm_arcade_anim_combat.c
    src/core/arcade/wm_arcade_attach_anim.c
    src/core/arcade/wm_arcade_bam.c
    src/core/arcade/wm_arcade_bret.c
    src/core/arcade/wm_arcade_bret_tables.c
    src/core/arcade/wm_arcade_combat.c
    src/core/arcade/wm_arcade_doink.c
    src/core/arcade/wm_arcade_drone.c
    src/core/arcade/wm_arcade_lex.c
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
    src/core/arcade/wmania_ring_out.c
    src/core/arcade/wmania_rng.c
    src/core/arcade/wmania_rope_command.c
    src/core/arcade/wmania_rope_runtime.c
    src/core/arcade/wmania_rope_source_data.c
    src/core/arcade/wmania_rope_spawn.c
)
# END FIX38 CUMULATIVE ARCADE SOURCE PORTS


target_include_directories(wmcore PUBLIC include src/fix39)
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
    add_executable(wm_fix39_tests tests/fix39_smoke.c)
    target_link_libraries(wm_fix39_tests PRIVATE wmcore)
    target_compile_options(wm_fix39_tests PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    add_test(NAME wm_fix39 COMMAND wm_fix39_tests)

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME wm_asmseq
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_asmseq.py)
    add_test(NAME wm_source_tools
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_source_tools.py)
endif()
