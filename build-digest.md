# Build digest

- Commit: `1acae7a1f5a4ca9290cdfbdd22784b74fb17de22`
- Branch: `duke/menu-centered-input-fixes`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40995346263
- Generated: 2026-10-30 03:05:55 UTC

Errors: **2**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 2 of 2 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2095),1`: expected expression

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2096),6`: expected expression

## Warnings

No warnings in the log.

## Log tail

```
[72/155] Building CXX object CMakeFiles\game.dir\qcommon\dnmap.cpp.obj
[73/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dnmath.cpp.obj
[74/155] Building CXX object CMakeFiles\cgame.dir\cgame\dnweaponanim.cpp.obj
[75/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dnanimation.cpp.obj
[76/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dntransport.cpp.obj
[77/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dnplayermove.cpp.obj
[78/155] Building CXX object CMakeFiles\cgame.dir\game\q_shared.cpp.obj
[79/155] Building CXX object CMakeFiles\ui.dir\ui\ui_duke_menus.cpp.obj
[80/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dncon.cpp.obj
[81/155] Building CXX object CMakeFiles\ui.dir\ui\ui_main.cpp.obj
[82/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dnmap.cpp.obj
[83/155] Building CXX object CMakeFiles\ui.dir\ui\ui_util.cpp.obj
[84/155] Building CXX object CMakeFiles\ui.dir\ui\ui_duke_menus2.cpp.obj
[85/155] Building CXX object CMakeFiles\ui.dir\game\q_shared.cpp.obj
[86/155] Building CXX object CMakeFiles\ui.dir\ui\ui_shared.cpp.obj
FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2095,1): error: expected expression
 2095 | \                               } else {
      | ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2096,6): error: expected expression
 2096 |                         } else {
      |                           ^
2 errors generated.
[87/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj
[88/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[89/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[90/155] Building CXX object CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj
[91/155] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

