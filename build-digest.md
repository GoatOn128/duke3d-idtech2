# Build digest

- Commit: `6ea8fb5c052e9e0b57caf4d89e2f633ee6c37ec5`
- Branch: `ci-build-digest`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40783958178
- Generated: 2026-10-28 22:59:17 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp(1736),1`: extraneous closing brace ('}')

  ```
  FAILED: [code=1] CMakeFiles/duke3d.dir/client/cl_tent.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj.d /FoCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp
  ```


## Warnings

No warnings in the log.

## Log tail

```
[82/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_ents.cpp.obj
[83/140] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[84/140] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[85/140] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\ref_gl.dll
[86/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_input.cpp.obj
[87/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_inv.cpp.obj
[88/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_fx.cpp.obj
[89/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_pred.cpp.obj
[90/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_parse.cpp.obj
[91/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_main.cpp.obj
[92/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_newfx.cpp.obj
[93/140] Building CXX object CMakeFiles\duke3d.dir\client\dnmenu.cpp.obj
[94/140] Building CXX object CMakeFiles\duke3d.dir\client\dnhud.cpp.obj
[95/140] Building CXX object CMakeFiles\duke3d.dir\client\dnsavemenu.cpp.obj
[96/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_scrn.cpp.obj
[97/140] Building CXX object CMakeFiles\duke3d.dir\client\dnintro.cpp.obj
[98/140] Building CXX object CMakeFiles\duke3d.dir\client\dnintermission.cpp.obj
[99/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_tent.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/client/cl_tent.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj.d /FoCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp(1736,1): error: extraneous closing brace ('}')
 1736 | }
      | ^
1 error generated.
[100/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_view.cpp.obj
[101/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\cmd.cpp.obj
[102/140] Building CXX object CMakeFiles\duke3d.dir\client\dncgame.cpp.obj
[103/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\common.cpp.obj
[104/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\cmodel.cpp.obj
ninja: build stopped: subcommand failed.
```

