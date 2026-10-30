# Build digest

- Commit: `c43f87980cde95785d0946cf9630b0010cb16daf`
- Branch: `feat/it4-cvar-server`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/41109275673
- Generated: 2026-10-30 18:44:04 UTC

Errors: **5**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

- `lld-link: error: undefined symbol: struct cvar_s *allow_download`
- `lld-link: error: undefined symbol: struct cvar_s *allow_download_maps`
- `lld-link: error: undefined symbol: struct cvar_s *allow_download_sounds`
- `lld-link: error: undefined symbol: struct cvar_s *allow_download_models`
- `lld-link: error: undefined symbol: struct cvar_s *allow_download_players`
## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```
lld-link: error: undefined symbol: struct cvar_s *allow_download_maps

>>> referenced by CMakeFiles\duke3d.dir\client\cl_main.cpp.obj:(void __cdecl CL_RequestNextDownload(void))

>>> referenced by CMakeFiles\duke3d.dir\client\cl_main.cpp.obj:(void __cdecl CL_RequestNextDownload(void))

>>> referenced by CMakeFiles\duke3d.dir\client\cl_main.cpp.obj:(void __cdecl CL_RequestNextDownload(void))



lld-link: error: undefined symbol: struct cvar_s *allow_download_sounds

>>> referenced by CMakeFiles\duke3d.dir\client\cl_main.cpp.obj:(void __cdecl CL_RequestNextDownload(void))

>>> referenced by CMakeFiles\duke3d.dir\client\cl_main.cpp.obj:(void __cdecl CL_RequestNextDownload(void))



lld-link: error: undefined symbol: struct cvar_s *allow_download_models

>>> referenced by CMakeFiles\duke3d.dir\client\cl_main.cpp.obj:(void __cdecl CL_RequestNextDownload(void))



lld-link: error: undefined symbol: struct cvar_s *allow_download_players

>>> referenced by CMakeFiles\duke3d.dir\client\cl_main.cpp.obj:(void __cdecl CL_RequestNextDownload(void))

[155/156] Building C object CMakeFiles\convertgrp.dir\tools\convertGRP\convert_grp.c.obj
ninja: build stopped: subcommand failed.
```

