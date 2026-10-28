cmake_minimum_required(VERSION 3.24)
project(
  Duke3D
  VERSION 0.1
  LANGUAGES C CXX RC)
if(NOT WIN32)
  message(FATAL_ERROR "The platform layer currently requires Windows.")
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(
    FATAL_ERROR "Duke3D requires Clang. Run build.ps1 or select clang-cl.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(DUKE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DUKE_ROOT}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DUKE_ROOT}")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} "${DUKE_ROOT}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config} "${DUKE_ROOT}")
endforeach()
add_compile_definitions(WIN32 _WINDOWS C_ONLY _CRT_SECURE_NO_WARNINGS
                        _CRT_NONSTDC_NO_WARNINGS)
add_compile_options(-Wno-microsoft-anon-tag -Wno-deprecated-declarations
                    -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0)
set(engine_sources
    client/cl_cin.cpp
    client/cl_ents.cpp
    client/cl_fx.cpp
    client/cl_input.cpp
    client/cl_inv.cpp
    client/cl_main.cpp
    client/cl_newfx.cpp
    client/cl_parse.cpp
    client/cl_pred.cpp
    client/dncgame.cpp
    client/cl_scrn.cpp
    client/dnhud.cpp
    client/dnmenu.cpp
    client/dnsavemenu.cpp
    client/dnintro.cpp
    client/dnintermission.cpp
    client/cl_tent.cpp
    client/cl_view.cpp
    qcommon/cmd.cpp
    qcommon/cmodel.cpp
    qcommon/common.cpp
    win32/conproc.cpp
    client/console.cpp
    qcommon/crc.cpp
    qcommon/cvar.cpp
    qcommon/files.cpp
    win32/in_win.cpp
    client/keys.cpp
    game/m_flash.cpp
    qcommon/md4.cpp
    client/menu.cpp
    qcommon/net_chan.cpp
    win32/net_wins.cpp
    qcommon/pmove.cpp
    game/q_shared.cpp
    win32/q_shwin.cpp
    client/qmenu.cpp
    sound/sndopenal.cpp
    sound/dnambience.cpp
    sound/sndwave.cpp
    sound/sndmusic.cpp
    mp3/dnmp3.cpp
    server/sv_ccmds.cpp
    server/sv_ents.cpp
    server/sv_game.cpp
    server/sv_init.cpp
    server/sv_main.cpp
    server/sv_send.cpp
    server/sv_user.cpp
    server/sv_world.cpp
    win32/sys_win.cpp
    win32/vid_dll.cpp
    win32/vid_menu.cpp
    client/x86.cpp)
set(renderer_sources
    ref_gl/gl_draw.cpp
    ref_gl/gl_image.cpp
    ref_gl/gl_light.cpp
    ref_gl/gl_mesh.cpp
    ref_gl/gl_model.cpp
    ref_gl/gl_rmain.cpp
    ref_gl/gl_rmisc.cpp
    ref_gl/gl_rsurf.cpp
    ref_gl/gl_warp.cpp
    win32/glw_imp.cpp
    game/q_shared.cpp
    win32/q_shwin.cpp
    win32/qgl_win.cpp)
add_executable(
  duke3d WIN32
  ${engine_sources}
  qcommon/dnmap.cpp
  qcommon/dntransport.cpp
  tests/dnnetwork.cpp
  tests/dnsave.cpp
  tests/dnroof.cpp
  win32/dnicon.rc)
# Build vendored OpenAL Soft statically with WASAPI.
set(LIBTYPE STATIC CACHE STRING "" FORCE)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_UPDATE_BUILD_VERSION OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_DSOUND OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_WINMM OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_WASAPI ON CACHE BOOL "" FORCE)
set(ALSOFT_EAX OFF CACHE BOOL "" FORCE)
add_subdirectory(openal EXCLUDE_FROM_ALL)
target_link_libraries(duke3d PRIVATE OpenAL winmm wsock32 user32 gdi32 advapi32)
add_library(
  ref_gl SHARED
  ${renderer_sources}
  qcommon/dnmap.cpp
  qcommon/dntransport.cpp
  qcommon/dnpcx.cpp
  qcommon/dnmath.cpp
  ref_gl/dnrender.cpp
  qcommon/dnvisibility.cpp
  ref_gl/ref_gl.def)
target_compile_definitions(ref_gl PRIVATE BUILDING_REF_GL)
target_link_libraries(ref_gl PRIVATE winmm user32 gdi32 opengl32 glu32)
add_library(
  game SHARED
  game/dnmain.cpp
  game/dnrandom.cpp
  game/dnassets.cpp
  game/dnentity.cpp
  qcommon/dnenemies.cpp
  game/dnlighting.cpp
  game/dnlights.cpp
  game/dndrop.cpp
  game/dnspawn.cpp
  game/dnintermission.cpp
  game/dnplayer.cpp
  game/dnmultiplayer.cpp
  game/dnweapons.cpp
  game/dncombat.cpp
  game/dndamage.cpp
  game/dnprojectiles.cpp
  game/dnactors.cpp
  game/dnalert.cpp
  game/dnphysics.cpp
  qcommon/dnzrange.cpp
  game/dnmove.cpp
  game/dnlaser.cpp
  game/dnenemyshots.cpp
  game/dnnativeai.cpp
  game/dnsound.cpp
  game/dnsectors.cpp
  game/dntransport.cpp
  game/dnstyles.cpp
  game/dndestruct.cpp
    game/dnwallhit.cpp
  game/dnfallers.cpp
  game/dnvehicles.cpp
  game/dneffects.cpp
  game/dndecals.cpp
  game/dngibs.cpp
  game/dnmapspawn.cpp
  game/dnuse.cpp
  game/dnsave.cpp
  game/dncommands.cpp
  qcommon/dncon.cpp
  game/dnconvm.cpp
  game/dnconrun.cpp
  qcommon/dnanimation.cpp
  qcommon/dnmath.cpp
  qcommon/dnplayermove.cpp
  qcommon/dnmap.cpp
  qcommon/dntransport.cpp
  game/q_shared.cpp
  game/game.def)
set_target_properties(
  game
  PROPERTIES OUTPUT_NAME dngamex86
             RUNTIME_OUTPUT_DIRECTORY "${DUKE_ROOT}/basedn"
             LIBRARY_OUTPUT_DIRECTORY "${DUKE_ROOT}/basedn"
             PDB_OUTPUT_DIRECTORY "${DUKE_ROOT}/basedn")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set_target_properties(
    game
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${DUKE_ROOT}/basedn"
               LIBRARY_OUTPUT_DIRECTORY_${config} "${DUKE_ROOT}/basedn"
               PDB_OUTPUT_DIRECTORY_${config} "${DUKE_ROOT}/basedn")
endforeach()
add_library(
  cgame SHARED
  cgame/dnmain.cpp
  qcommon/dnenemies.cpp
  cgame/dnweaponview.cpp
  qcommon/dnzrange.cpp
  cgame/dnanimation.cpp
  cgame/dnweaponanim.cpp
  qcommon/dnplayermove.cpp
  qcommon/dnmath.cpp
  qcommon/dnanimation.cpp
  qcommon/dncon.cpp
  qcommon/dnmap.cpp
  qcommon/dntransport.cpp
  game/q_shared.cpp
  cgame/cgame.def)
set_target_properties(
  cgame
  PROPERTIES OUTPUT_NAME cgamex86
             RUNTIME_OUTPUT_DIRECTORY "${DUKE_ROOT}/basedn"
             LIBRARY_OUTPUT_DIRECTORY "${DUKE_ROOT}/basedn"
             PDB_OUTPUT_DIRECTORY "${DUKE_ROOT}/basedn")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set_target_properties(
    cgame
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${DUKE_ROOT}/basedn"
               LIBRARY_OUTPUT_DIRECTORY_${config} "${DUKE_ROOT}/basedn"
               PDB_OUTPUT_DIRECTORY_${config} "${DUKE_ROOT}/basedn")
endforeach()

target_link_libraries(game PRIVATE winmm user32)
add_dependencies(duke3d ref_gl game cgame)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(
  OUTPUT "${DUKE_ROOT}/basedn/asset-manifest.json"
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_assets.py" --root "${DUKE_ROOT}"
  DEPENDS tools/prepare_assets.py tools/dnintro_assets.py
          "${DUKE_ROOT}/3drealms/duke3d.grp"
  COMMENT
    "Preparing Duke maps, WAL textures, palette, voices and console assets"
  VERBATIM)
add_custom_target(duke_assets DEPENDS "${DUKE_ROOT}/basedn/asset-manifest.json")
add_dependencies(duke3d duke_assets)
include(CTest)
if(BUILD_TESTING)
  add_executable(duke_audio_tests tests/dnaudio_test.cpp sound/sndwave.cpp mp3/dnmp3.cpp)
  target_link_libraries(duke_audio_tests PRIVATE OpenAL)
  set_target_properties(duke_audio_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                        "${CMAKE_BINARY_DIR}/tests")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(duke_audio_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config}
                          "${CMAKE_BINARY_DIR}/tests")
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
      "${CMAKE_BINARY_DIR}/reference/dnshadepool.inc")
  list(APPEND dn_sector_references "${CMAKE_BINARY_DIR}/reference/dnsway.inc")
  list(APPEND dn_sector_references
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
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_references.py"
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
  add_executable(
    duke_game_tests
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
  set_target_properties(duke_game_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                   "${CMAKE_BINARY_DIR}/tests")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(
      duke_game_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config}
                                 "${CMAKE_BINARY_DIR}/tests")
  endforeach()
  add_test(NAME duke_game_interactions
           COMMAND duke_game_tests "${DUKE_ROOT}" "${CMAKE_BINARY_DIR}"
                   interactions)
  add_test(NAME duke_demolition COMMAND duke_game_tests "${DUKE_ROOT}"
                                        "${CMAKE_BINARY_DIR}" demolition)
  add_test(NAME duke_glass COMMAND duke_game_tests "${DUKE_ROOT}"
                                    "${CMAKE_BINARY_DIR}" glass)
  add_test(NAME duke_con_runtime COMMAND duke_game_tests "${DUKE_ROOT}"
                                         "${CMAKE_BINARY_DIR}" con)
  add_test(NAME duke_actor_behaviors COMMAND duke_game_tests "${DUKE_ROOT}"
                                             "${CMAKE_BINARY_DIR}" actors)
  add_test(NAME duke_trooper_behaviors COMMAND duke_game_tests "${DUKE_ROOT}"
                                               "${CMAKE_BINARY_DIR}" trooper)
  add_test(NAME duke_enemy_ai COMMAND duke_game_tests "${DUKE_ROOT}"
                                 "${CMAKE_BINARY_DIR}" ai)
  add_test(NAME duke_player_physics COMMAND duke_game_tests "${DUKE_ROOT}"
                                            "${CMAKE_BINARY_DIR}" player)
  add_test(NAME duke_sector_motion COMMAND duke_game_tests "${DUKE_ROOT}"
                                           "${CMAKE_BINARY_DIR}" sectors)
  add_test(NAME duke_sprite_lighting COMMAND duke_game_tests "${DUKE_ROOT}"
                                             "${CMAKE_BINARY_DIR}" lighting)
  add_test(NAME duke_level_restart COMMAND duke_game_tests "${DUKE_ROOT}"
                                           "${CMAKE_BINARY_DIR}" restart)
  add_test(NAME duke_multiplayer_spawning COMMAND duke_game_tests "${DUKE_ROOT}"
                                                  "${CMAKE_BINARY_DIR}" multiplayer)
  add_test(NAME duke_level_results COMMAND duke_game_tests "${DUKE_ROOT}"
                                           "${CMAKE_BINARY_DIR}" intermission)
  add_test(NAME duke_actor_smoke COMMAND duke_game_tests "${DUKE_ROOT}"
                                         "${CMAKE_BINARY_DIR}" actor-smoke)
  add_test(NAME duke_cgame_runtime COMMAND duke_game_tests "${DUKE_ROOT}"
                                           "${CMAKE_BINARY_DIR}" cgame)
  add_executable(
    duke_tests
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
  set_target_properties(duke_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                              "${CMAKE_BINARY_DIR}/tests")
  target_include_directories(duke_tests PRIVATE "${CMAKE_BINARY_DIR}/reference")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(
      duke_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config}
                            "${CMAKE_BINARY_DIR}/tests")
  endforeach()
  file(
    GLOB duke_test_maps
    RELATIVE "${DUKE_ROOT}/basedn"
    "${DUKE_ROOT}/basedn/maps/*.map")
  add_test(NAME duke_native_formats COMMAND duke_tests "${DUKE_ROOT}/basedn"
                                            ${duke_test_maps})
  add_test(NAME duke_network_delta COMMAND duke3d +set dedicated 1 +dn_netcheck
                                           +quit)
  add_test(NAME duke_save_restore COMMAND duke3d +set dedicated 1 +dn_savecheck
                                          +quit)
  add_test(NAME duke_rooftop_collision COMMAND duke3d +set dedicated 1
                                               +dn_roofcheck +quit)
  set_tests_properties(duke_network_delta duke_save_restore
                       duke_rooftop_collision PROPERTIES TIMEOUT 15)
endif()
