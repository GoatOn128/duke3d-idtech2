# Vendored dependencies. Everything the engine needs ships inside
# code/third-party; nothing is fetched at configure or build time.
#
#   third-party/openal-soft  LGPL (prebuilt import lib + soft_oal.dll)
#   third-party/sdl3        zlib (prebuilt, not linked by the engine yet)
#   third-party/glad        MIT (GL loader source, not linked yet)
#   third-party/mp3         MIT (dr_mp3 single-header decoder)
#
# OpenAL Soft is vendored as the official prebuilt distribution. Linking is
# dynamic, which keeps the LGPL obligations minimal (ship the DLL and its
# license; the repo already carries COPYING).

if(DN_ARCH STREQUAL "x64")
  set(DN_OPENAL_LIB_DIR "${DUKE_THIRD_PARTY_DIR}/openal-soft/libs/Win64")
  set(DN_OPENAL_DLL "${DUKE_THIRD_PARTY_DIR}/openal-soft/bin/Win64/soft_oal.dll")
else()
  set(DN_OPENAL_LIB_DIR "${DUKE_THIRD_PARTY_DIR}/openal-soft/libs/Win32")
  set(DN_OPENAL_DLL "${DUKE_THIRD_PARTY_DIR}/openal-soft/bin/Win32/soft_oal.dll")
endif()

if(NOT EXISTS "${DN_OPENAL_LIB_DIR}/OpenAL32.lib")
  message(
    FATAL_ERROR
      "Missing ${DN_OPENAL_LIB_DIR}/OpenAL32.lib for DN_ARCH=${DN_ARCH}. "
      "Check the vendored OpenAL Soft distribution.")
endif()

add_library(OpenAL SHARED IMPORTED)
set(duke_openal_properties
  IMPORTED_IMPLIB "${DN_OPENAL_LIB_DIR}/OpenAL32.lib"
  IMPORTED_LOCATION "${DN_OPENAL_DLL}"
  INTERFACE_INCLUDE_DIRECTORIES "${DUKE_THIRD_PARTY_DIR}/openal-soft/include")
# The vendored OpenAL32.lib import library ships without SEH unwind info
# and lld-link turns the SafeSEH check on by default for x86, so consumers
# must opt out of the check when linking against it.
if(DN_ARCH STREQUAL "x86")
  list(APPEND duke_openal_properties INTERFACE_LINK_OPTIONS "/SAFESEH:NO")
endif()
set_target_properties(OpenAL PROPERTIES ${duke_openal_properties})

# Windows resolves the import lib against OpenAL32.dll, so the vendored
# soft_oal.dll has to be shipped under that name next to the executable.
function(duke_copy_openal_dll target)
  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different "${DN_OPENAL_DLL}"
      "$<TARGET_FILE_DIR:${target}>/OpenAL32.dll"
    COMMENT "Shipping soft_oal.dll as OpenAL32.dll")
endfunction()
