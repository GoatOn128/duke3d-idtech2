# Build digest

- Commit: `8f071832f912ad72ce3c612f94b369fe2b55e24d`
- Branch: `feat/it4-cvar-system`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/41104476810
- Generated: 2026-10-30 18:10:47 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\cvar.cpp(602),3`: no matching function for call to 'Cvar_Set'

  ```
  FAILED: [code=1] CMakeFiles/duke3d.dir/qcommon/cvar.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj.d /FoCMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\cvar.cpp
  ```


## Warnings

No warnings in the log.

## Log tail

```
[101/156] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.dll
[102/156] Building CXX object CMakeFiles\duke3d.dir\client\dnmenu.cpp.obj
[103/156] Building CXX object CMakeFiles\duke3d.dir\client\dnintro.cpp.obj
[104/156] Building CXX object CMakeFiles\duke3d.dir\client\dnintermission.cpp.obj
[105/156] Building CXX object CMakeFiles\duke3d.dir\client\dncgame.cpp.obj
[106/156] Building CXX object CMakeFiles\duke3d.dir\client\dnui.cpp.obj
[107/156] Building CXX object CMakeFiles\duke3d.dir\client\roq.cpp.obj
[108/156] Building CXX object CMakeFiles\duke3d.dir\qcommon\cmd.cpp.obj
[109/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_tent.cpp.obj
[110/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_view.cpp.obj
[111/156] Building CXX object CMakeFiles\duke3d.dir\client\dndebuggraph.cpp.obj
[112/156] Building CXX object CMakeFiles\duke3d.dir\qcommon\cmodel.cpp.obj
[113/156] Building CXX object CMakeFiles\duke3d.dir\qcommon\common.cpp.obj
[114/156] Building CXX object CMakeFiles\duke3d.dir\client\console.cpp.obj
[115/156] Building CXX object CMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/qcommon/cvar.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj.d /FoCMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\cvar.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\cvar.cpp(602,3): error: no matching function for call to 'Cvar_Set'
  602 |                 Cvar_Set (var->name, var->value != 0.0f ? "0" : "1");
      |                 ^~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\cvar.cpp(352,9): note: candidate function not viable: no known conversion from 'const char[2]' to 'char *' for 2nd argument
  352 | cvar_t *Cvar_Set (char *var_name, char *value)
      |         ^                         ~~~~~~~~~~~
1 error generated.
[116/156] Building CXX object CMakeFiles\duke3d.dir\qcommon\crc.cpp.obj
[117/156] Building CXX object CMakeFiles\duke3d.dir\qcommon\files.cpp.obj
[118/156] Building CXX object CMakeFiles\duke3d.dir\win32\dn_syscon.cpp.obj
[119/156] Building CXX object CMakeFiles\duke3d.dir\sdl\sdl_input.cpp.obj
[120/156] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

