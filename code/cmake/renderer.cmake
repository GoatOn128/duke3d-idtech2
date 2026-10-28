# ref_gl.dll: the OpenGL renderer module.

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
duke_set_output_directories(ref_gl "${DUKE_ROOT}")
