# Build digest

- Commit: `c69d1cd53a0b8e3ad408126e188ebedb498af20d`
- Branch: `agent-docs`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40785281210
- Generated: 2026-10-28 23:10:19 UTC

Errors: **21**, warnings: **1**.

## CMake errors

No CMake errors in the log.

## Tool errors

- `lld-link: error: /safeseh: OpenAL32_lib_s00126.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00125.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00113.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00112.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00111.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00103.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00101.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00100.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00098.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00095.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00093.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00091.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00089.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00086.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00085.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00083.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00075.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00073.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00053.o is not compatible with SEH`
- `lld-link: error: /safeseh: OpenAL32_lib_s00033.o is not compatible with SEH`
- `lld-link: error: too many errors emitted, stopping now (use /errorlimit:0 to see all errors)`
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

lld-link: error: /safeseh: OpenAL32_lib_s00100.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00098.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00095.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00093.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00091.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00089.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00086.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00085.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00083.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00075.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00073.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00053.o is not compatible with SEH

lld-link: error: /safeseh: OpenAL32_lib_s00033.o is not compatible with SEH

lld-link: error: too many errors emitted, stopping now (use /errorlimit:0 to see all errors)

ninja: build stopped: subcommand failed.
```

