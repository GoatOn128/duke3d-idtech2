# Test suite. The tests reference code/tests/ and the 3D Realms reference
# sources, which are moving to references/ as part of the restructure (see
# PROGRESS.md). The suite is OFF by default until those files return, because
# enabling it without them can only fail; when they are back, remove the
# existence guard and re-enable the default.

# Default OFF until the tests restructure lands. (Must be set before the
# CTest include assigns its own default.)
if(NOT DEFINED BUILD_TESTING)
  set(BUILD_TESTING OFF)
endif()

include(CTest)

if(BUILD_TESTING)
  if(NOT EXISTS "${DUKE_CODE_DIR}/tests" OR NOT EXISTS "${DUKE_ROOT}/3drealms")
    message(
      FATAL_ERROR
        "BUILD_TESTING is ON but code/tests or the reference sources "
        "(3drealms/) are not present. Configure with -DBUILD_TESTING=OFF "
        "until the tests restructure lands.")
  endif()

  add_executable(duke_audio_tests
    tests/dnaudio_test.cpp
    sound/sndwave.cpp
    third-party/mp3/dnmp3.cpp)
  target_link_libraries(duke_audio_tests PRIVATE OpenAL)
  set_target_properties(duke_audio_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(duke_audio_tests PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/tests")
  endforeach()
  add_test(NAME duke_audio COMMAND duke_audio_tests "${DUKE_ROOT}/basedn")
  set_tests_properties(duke_audio PROPERTIES TIMEOUT 120)

  set(dn_seenine_reference "${CMAKE_BINARY_DIR}/reference/dnseenine.inc")
  set(dn_transport_reference "${CMAKE_BINARY_DIR}/reference/dntransport.inc")
  set(dn_movement_reference "${CMAKE_BINARY_DIR}/reference/dnmovement.inc")
  set(dn_sprite_reference "${CMAKE_BINARY_DIR}/reference/dnspriteview.inc")
  set(dn_player_vertical_reference
    "${CMAKE_BINARY_DIR}/reference/dnplayervertical.inc")
  set(dn_player_horizontal_reference
    "${CMAKE_BINARY_DIR}/reference/dnplayerhorizontal.inc")
  set(dn_gibs_reference "${CMAKE_BINARY_DIR}/reference/dngibs.inc")
  set(dn_sector_references
    "${CMAKE_BINARY_DIR}/reference/dnplatform.inc"
    "${CMAKE_BINARY_DIR}/reference/dnneighbor.inc"
    "${CMAKE_BINARY_DIR}/reference/dnrotate.inc"
    "${CMAKE_BINARY_DIR}/reference/dndragpoint.inc"
    "${CMAKE_BINARY_DIR}/reference/dnshade.inc"
    "${CMAKE_BINARY_DIR}/reference/dnshadepool.inc"
    "${CMAKE_BINARY_DIR}/reference/dnsway.inc"
    "${CMAKE_BINARY_DIR}/reference/dnfallclock.inc"
    "${CMAKE_BINARY_DIR}/reference/dnblockedturn.inc")
  add_custom_command(
    OUTPUT "${dn_seenine_reference}"
           "${dn_transport_reference}"
           "${dn_movement_reference}"
           "${dn_sprite_reference}"
           "${dn_player_vertical_reference}"
           "${dn_player_horizontal_reference}"
           "${dn_gibs_reference}"
           ${dn_sector_references}
    COMMAND "${Python3_EXECUTABLE}"
            "${DUKE_CODE_DIR}/tools/prepare_references.py"
            "${DUKE_ROOT}/3drealms/source/ACTORS.cpp" "${dn_seenine_reference}"
    DEPENDS tools/prepare_references.py
            "${DUKE_ROOT}/3drealms/source/ACTORS.cpp"
            "${DUKE_ROOT}/3drealms/source/GAMEDEF.cpp"
            "${DUKE_ROOT}/3drealms/source/GAME.cpp"
            "${DUKE_ROOT}/3drealms/source/PLAYER.cpp"
            "${DUKE_ROOT}/3drealms/source/SECTOR.cpp"
            "${DUKE_ROOT}/3drealms/SRC/ENGINE.cpp"
    VERBATIM)

  get_target_property(dn_game_test_sources game SOURCES)
  list(REMOVE_ITEM dn_game_test_sources game/game.def)
  add_executable(duke_game_tests
    tests/dninteractions.cpp
    tests/dndestruct_test.cpp
    tests/dnwallhit_test.cpp
    tests/dncon_test.cpp
    tests/dnactors_test.cpp
    tests/dndecals_test.cpp
    tests/dntrooper_test.cpp
    tests/dnai_test.cpp
    tests/dnmovementref.cpp
    tests/dnplayerref.cpp
    tests/dnplayer_test.cpp
    tests/dngibref.cpp
    tests/dngibs_test.cpp
    tests/dncgame_test.cpp
    tests/dnsectors_test.cpp
    tests/dndrop_test.cpp
    tests/dnsectorref.cpp
    tests/dnshade_test.cpp
    tests/dnrestart_test.cpp
    tests/dnmultiplayer_test.cpp
    tests/dnintermission_test.cpp
    tests/dnseenineref.cpp
    "${dn_seenine_reference}"
    "${dn_movement_reference}"
    "${dn_sprite_reference}"
    "${dn_player_vertical_reference}"
    "${dn_player_horizontal_reference}"
    "${dn_gibs_reference}"
    ${dn_sector_references}
    ${dn_game_test_sources})
  target_include_directories(duke_game_tests
    PRIVATE "${CMAKE_BINARY_DIR}/reference")
  target_link_libraries(duke_game_tests PRIVATE winmm user32)
  add_dependencies(duke_game_tests cgame)
  set_target_properties(duke_game_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(duke_game_tests PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/tests")
  endforeach()

  add_test(NAME duke_game_interactions
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" interactions)
  add_test(NAME duke_demolition
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" demolition)
  add_test(NAME duke_glass
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" glass)
  add_test(NAME duke_con_runtime
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" con)
  add_test(NAME duke_actor_behaviors
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" actors)
  add_test(NAME duke_trooper_behaviors
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" trooper)
  add_test(NAME duke_enemy_ai
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" ai)
  add_test(NAME duke_player_physics
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" player)
  add_test(NAME duke_sector_motion
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" sectors)
  add_test(NAME duke_sprite_lighting
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" lighting)
  add_test(NAME duke_level_restart
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" restart)
  add_test(NAME duke_multiplayer_spawning
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" multiplayer)
  add_test(NAME duke_level_results
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" intermission)
  add_test(NAME duke_actor_smoke
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" actor-smoke)
  add_test(NAME duke_cgame_runtime
    COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}" cgame)

  add_executable(duke_tests
    tests/dntests.cpp
    tests/dntransportref.cpp
    "${dn_transport_reference}"
    qcommon/dnmap.cpp
    qcommon/dntransport.cpp
    qcommon/dnvisibility.cpp
    qcommon/dnmath.cpp
    qcommon/dncon.cpp
    game/dnconvm.cpp
    game/dnrandom.cpp
    cgame/dnweaponview.cpp
    qcommon/dnpcx.cpp
    game/q_shared.cpp)
  set_target_properties(duke_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
  target_include_directories(duke_tests
    PRIVATE "${CMAKE_BINARY_DIR}/reference")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(duke_tests PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/tests")
  endforeach()

  file(GLOB duke_test_maps
    RELATIVE "${DUKE_ROOT}/basedn"
    "${DUKE_ROOT}/basedn/maps/*.map")
  add_test(NAME duke_native_formats
    COMMAND duke_tests "${DUKE_ROOT}/basedn" ${duke_test_maps})

  add_test(NAME duke_network_delta
    COMMAND duke3d +set dedicated 1 +dn_netcheck +quit)
  add_test(NAME duke_save_restore
    COMMAND duke3d +set dedicated 1 +dn_savecheck +quit)
  add_test(NAME duke_rooftop_collision
    COMMAND duke3d +set dedicated 1 +dn_roofcheck +quit)
  set_tests_properties(duke_network_delta duke_save_restore
    duke_rooftop_collision PROPERTIES TIMEOUT 15)
endif()
