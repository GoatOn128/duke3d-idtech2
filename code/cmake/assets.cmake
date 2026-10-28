# Asset preparation (maps, WAL textures, palette, voices, console assets) is
# opt-in and separate from building the binaries: nothing in the default build
# depends on it.
#
# The pipeline needs Python, the asset scripts under code/tools/ and the game
# data (3drealms/duke3d.grp). None of those are in the repo right now; they
# return with the asset pipeline restructure (see PROGRESS.md). Until then
# configuring simply reports that the step is skipped.

find_package(Python3 COMPONENTS Interpreter)

if(Python3_Interpreter_FOUND
   AND EXISTS "${DUKE_CODE_DIR}/tools/prepare_assets.py"
   AND EXISTS "${DUKE_ROOT}/3drealms/duke3d.grp")
  add_custom_command(
    OUTPUT "${DUKE_ROOT}/basedn/asset-manifest.json"
    COMMAND "${Python3_EXECUTABLE}"
            "${DUKE_CODE_DIR}/tools/prepare_assets.py"
            --root "${DUKE_ROOT}"
    DEPENDS tools/prepare_assets.py
            tools/dnintro_assets.py
            "${DUKE_ROOT}/3drealms/duke3d.grp"
    COMMENT "Preparing Duke maps, WAL textures, palette, voices and console assets"
    VERBATIM)
  add_custom_target(duke_assets
    DEPENDS "${DUKE_ROOT}/basedn/asset-manifest.json")
else()
  message(
    STATUS
      "Asset preparation skipped: needs Python, code/tools/prepare_assets.py "
      "and 3drealms/duke3d.grp.")
endif()
