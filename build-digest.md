# Build digest

- Commit: `65999fa0b0c030c602ea62ca9bf7a13766ff754d`
- Branch: `agent-docs`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40782384061
- Generated: 2026-10-28 22:46:18 UTC

Errors: **0**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```

>>> referenced 3 more times



lld-link: error: undefined symbol: void (__stdcall *qglEnableClientState)(unsigned int)

>>> referenced by CMakeFiles\ref_gl.dir\ref_gl\gl_mesh.cpp.obj:(void __cdecl GL_DrawAliasFrameLerp(struct dmdl_t *, float))

>>> referenced by CMakeFiles\ref_gl.dir\ref_gl\gl_mesh.cpp.obj:(void __cdecl GL_DrawAliasFrameLerp(struct dmdl_t *, float))



lld-link: error: too many errors emitted, stopping now (use /errorlimit:0 to see all errors)

[82/139] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[83/139] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[84/139] Building CXX object CMakeFiles\duke3d.dir\client\cl_ents.cpp.obj
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711,9): warning: using floating point absolute value function 'fabs' when argument is of integer type [-Wabsolute-value]
 1711 |     if (fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256 * 8 ||
      |         ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711,9): note: use function 'std::abs' instead
 1711 |     if (fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256 * 8 ||
      |         ^~~~
      |         std::abs
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711,9): note: include the header <cstdlib> or explicitly provide a declaration for 'std::abs'
1 warning generated.
[85/139] Building CXX object CMakeFiles\duke3d.dir\client\cl_input.cpp.obj
[86/139] Building CXX object CMakeFiles\duke3d.dir\client\cl_fx.cpp.obj
ninja: build stopped: subcommand failed.
```

