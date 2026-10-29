# Build digest

- Commit: `0bb4e971b60423cc3a7beba0e862c57f99d25d5b`
- Branch: `feat/ui-syscalls`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40925738450
- Generated: 2026-10-29 17:55:16 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\dnui.cpp(73),5`: use of undeclared identifier 'Cmd_ExecuteText'

  ```
  FAILED: [code=1] CMakeFiles/duke3d.dir/client/dnui.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\client\dnui.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\client\dnui.cpp.obj.d /FoCMakeFiles\duke3d.dir\client\dnui.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\client\dnui.cpp
  ```


## Warnings

No warnings in the log.

## Log tail

```
[85/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[86/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[87/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.dll
[88/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_inv.cpp.obj
[89/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_input.cpp.obj
[90/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_fx.cpp.obj
[91/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_newfx.cpp.obj
[92/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_main.cpp.obj
[93/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_pred.cpp.obj
[94/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_parse.cpp.obj
[95/154] Building CXX object CMakeFiles\duke3d.dir\client\dnmenu.cpp.obj
[96/154] Building CXX object CMakeFiles\duke3d.dir\client\dnhud.cpp.obj
[97/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_scrn.cpp.obj
[98/154] Building CXX object CMakeFiles\duke3d.dir\client\dnintro.cpp.obj
[99/154] Building CXX object CMakeFiles\duke3d.dir\client\dnsavemenu.cpp.obj
[100/154] Building CXX object CMakeFiles\duke3d.dir\client\dnintermission.cpp.obj
[101/154] Building CXX object CMakeFiles\duke3d.dir\client\dncgame.cpp.obj
[102/154] Building CXX object CMakeFiles\duke3d.dir\client\dnui.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/client/dnui.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\client\dnui.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\client\dnui.cpp.obj.d /FoCMakeFiles\duke3d.dir\client\dnui.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\client\dnui.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\dnui.cpp(73,5): error: use of undeclared identifier 'Cmd_ExecuteText'
   73 |     Cmd_ExecuteText(when, (char *)text);
      |     ^
1 error generated.
[103/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_view.cpp.obj
[104/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\cmd.cpp.obj
[105/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_tent.cpp.obj
[106/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\cmodel.cpp.obj
[107/154] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

