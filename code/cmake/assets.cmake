# Asset preparation (maps, WAL textures, palette, voices, console assets) is
# opt-in and separate from building the binaries: nothing in the default build
# depends on it.
#
# The converter itself builds everywhere; running it needs the game data
# (3drealms/duke3d.grp), which is not part of the repo. Until it is present
# configuring simply reports that the step is skipped.

add_executable(convertgrp tools/convertGRP/convert_grp.c)

if(EXISTS "${DUKE_ROOT}/3drealms/duke3d.grp")
  add_custom_command(
    OUTPUT "${DUKE_ROOT}/basedn/asset-manifest.json"
    COMMAND convertgrp --root "${DUKE_ROOT}"
    DEPENDS convertgrp
            tools/convertGRP/convert_grp.c
            "${DUKE_ROOT}/3drealms/duke3d.grp"
    COMMENT "Preparing Duke maps, WAL textures, palette, voices and console assets"
    VERBATIM)
  add_custom_target(duke_assets
    DEPENDS "${DUKE_ROOT}/basedn/asset-manifest.json")
else()
  message(STATUS "Asset preparation skipped: needs 3drealms/duke3d.grp.")
endif()
