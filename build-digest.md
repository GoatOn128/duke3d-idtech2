# Build digest

- Commit: `bc8ffe55b15528a6b8b59289b344322382cc91e7`
- Branch: `ci-build-digest`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40784733643
- Generated: 2026-10-28 23:05:39 UTC

Errors: **9**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

- `lld-link: error: undefined symbol: void __cdecl DN_NetworkTestFrame(void)`
- `lld-link: error: undefined symbol: void __cdecl DN_NetworkCheck_f(void)`
- `lld-link: error: undefined symbol: void __cdecl DN_SaveCheck_f(void)`
- `lld-link: error: undefined symbol: void __cdecl DN_MenuKeyCheck_f(void)`
- `lld-link: error: undefined symbol: void __cdecl DN_RoofCheck_f(void)`
- `lld-link: error: undefined symbol: void __cdecl DN_PredictionInfo_f(void)`
- `lld-link: error: undefined symbol: void __cdecl DN_InputCheck_f(void)`
- `lld-link: error: undefined symbol: void __cdecl DN_TestFrames_f(void)`
- `lld-link: error: undefined symbol: void __cdecl DN_WorldInfo_f(void)`
## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```

lld-link: error: undefined symbol: void __cdecl DN_RoofCheck_f(void)

>>> referenced by CMakeFiles\duke3d.dir\qcommon\common.cpp.obj:(void __cdecl Qcommon_Init(int, char **))



lld-link: error: undefined symbol: void __cdecl DN_PredictionInfo_f(void)

>>> referenced by CMakeFiles\duke3d.dir\qcommon\common.cpp.obj:(void __cdecl Qcommon_Init(int, char **))



lld-link: error: undefined symbol: void __cdecl DN_InputCheck_f(void)

>>> referenced by CMakeFiles\duke3d.dir\qcommon\common.cpp.obj:(void __cdecl Qcommon_Init(int, char **))



lld-link: error: undefined symbol: void __cdecl DN_TestFrames_f(void)

>>> referenced by CMakeFiles\duke3d.dir\qcommon\common.cpp.obj:(void __cdecl Qcommon_Init(int, char **))



lld-link: error: undefined symbol: void __cdecl DN_WorldInfo_f(void)

>>> referenced by CMakeFiles\duke3d.dir\qcommon\common.cpp.obj:(void __cdecl Qcommon_Init(int, char **))

ninja: build stopped: subcommand failed.
```

