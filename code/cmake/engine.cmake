# duke3d.exe: client, server, shared code, sound, mp3 decoder and the SDL3
# platform layer (win32/ keeps console, sockets, shell and icon).

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
    sdl/sdl_input.cpp
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
    third-party/mp3/dnmp3.cpp
    server/sv_ccmds.cpp
    server/sv_ents.cpp
    server/sv_game.cpp
    server/sv_init.cpp
    server/sv_main.cpp
    server/sv_send.cpp
    server/sv_user.cpp
    server/sv_world.cpp
    sdl/sdl_sys.cpp
    sdl/sdl_vid.cpp
    win32/vid_menu.cpp
    qcommon/dnmap.cpp
    qcommon/dnthreads.cpp
    qcommon/dnjobs.cpp
    qcommon/dntransport.cpp)

# The executable icon is the in-repo Quake 2 icon, compiled unconditionally.
list(APPEND engine_sources win32/dnicon.rc)

add_executable(duke3d WIN32 ${engine_sources})

target_link_libraries(duke3d PRIVATE OpenAL SDL3 wsock32 user32)
duke_set_output_directories(duke3d "${DUKE_ROOT}")
duke_copy_openal_dll(duke3d)
duke_copy_sdl3_dll(duke3d)

# The pre-restructure tree also compiled tests/dnnetwork.cpp, tests/dnsave.cpp
# and tests/dnroof.cpp into this executable. They return together with the
# test suite restructure (PROGRESS.md) and code/cmake/tests.cmake. Until then
# the engine guards their entry points behind the DN_TEST_SUPPORT macro, so
# re-add target_compile_definitions(duke3d PRIVATE DN_TEST_SUPPORT) here when
# the test sources come back.

add_dependencies(duke3d ref_gl game cgame)
