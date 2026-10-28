# Shared build helpers. The project files under cmake/ only describe sources,
# includes and linking; everything reusable lives here.

function(duke_apply_global_compile_settings)
  add_compile_definitions(WIN32 _WINDOWS C_ONLY _CRT_SECURE_NO_WARNINGS
                          _CRT_NONSTDC_NO_WARNINGS)
  # The legacy codebase uses anonymous tags, deprecated CRT calls, pragma pack
  # tricks and string literals assigned to char*. Keep the build verbose-proof
  # and never stop at the first error so the CI digest can list everything in
  # a single pass.
  add_compile_options(-Wno-microsoft-anon-tag
                      -Wno-deprecated-declarations
                      -Wno-pragma-pack
                      -Wno-writable-strings
                      -ferror-limit=0)
endfunction()

# clang-cl takes the target machine from the active MSVC environment
# (VS x86 prompt, tools/vsenv.cmd x86, or msvc-dev-cmd with arch: x86 in CI).
# Fail with a clear message instead of a cryptic linker error when the wrong
# environment is active.
function(duke_check_target_architecture)
  if(DN_ARCH STREQUAL "x86" AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(
      FATAL_ERROR
        "DN_ARCH is x86 but the toolchain produces ${CMAKE_SIZEOF_VOID_P}-byte "
        "pointers. Activate the VS x86 environment (call tools/vsenv.cmd x86) "
        "and reconfigure from a clean build directory.")
  elseif(DN_ARCH STREQUAL "x64" AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(
      FATAL_ERROR
        "DN_ARCH is x64 but the toolchain produces ${CMAKE_SIZEOF_VOID_P}-byte "
        "pointers. Activate the VS x64 environment (call tools/vsenv.cmd x64) "
        "and reconfigure from a clean build directory.")
  endif()
endfunction()

# Points RUNTIME/LIBRARY/PDB outputs of a target at one directory for every
# configuration. Replaces the per-config foreach blocks the old monolithic
# CMakeLists.txt repeated for each target.
function(duke_set_output_directories target dir)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${dir}"
    LIBRARY_OUTPUT_DIRECTORY "${dir}"
    PDB_OUTPUT_DIRECTORY "${dir}")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${config} "${dir}"
      LIBRARY_OUTPUT_DIRECTORY_${config} "${dir}"
      PDB_OUTPUT_DIRECTORY_${config} "${dir}")
  endforeach()
endfunction()

# Shared output pattern for the game modules the engine loads from basedn/.
function(duke_declare_game_module target output_name)
  set_target_properties(${target} PROPERTIES OUTPUT_NAME ${output_name})
  duke_set_output_directories(${target} "${DUKE_ROOT}/basedn")
endfunction()
