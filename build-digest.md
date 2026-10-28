# Build digest

- Commit: `6a477a2046d3abc00491aff295452103a407307d`
- Branch: `agent-docs`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40784758232
- Generated: 2026-10-28 23:05:57 UTC

Errors: **9**, warnings: **1**.

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

1 warning(s) in 1 kind(s):

| Kind | Count | Locations |
| --- | --- | --- |
| using floating | 1 | D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711) |

### Warning messages by kind

- **using floating**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711)`: using floating point absolute value function 'fabs' when argument is of integer type [-Wabsolute-value]

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

