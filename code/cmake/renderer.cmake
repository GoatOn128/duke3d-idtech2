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
    ref_gl/gl_shaders.cpp
    ref_gl/gl_warp.cpp
    ref_gl/glimp_sdl.cpp
    game/q_shared.cpp
    win32/q_shwin.cpp
    third-party/glad/src/gl.c)

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

target_include_directories(ref_gl PRIVATE third-party/glad/include)
target_compile_definitions(ref_gl PRIVATE BUILDING_REF_GL)
target_link_libraries(ref_gl PRIVATE SDL3 glu32)
duke_set_output_directories(ref_gl "${DUKE_ROOT}")
