# Build digest

- Commit: `30dd2b1240fe4521cb76e7256157dcf6eac4e235`
- Branch: `fix/ui-null-item-names`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40954439232
- Generated: 2026-10-29 21:04:17 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2448),1`: expected '}'

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
  ```


## Warnings

No warnings in the log.

## Log tail

```
[70/154] Building CXX object CMakeFiles\game.dir\game\q_shared.cpp.obj
[71/154] Building CXX object CMakeFiles\game.dir\qcommon\dnmap.cpp.obj
[72/154] Building CXX object CMakeFiles\cgame.dir\qcommon\dnmath.cpp.obj
[73/154] Building CXX object CMakeFiles\cgame.dir\qcommon\dnanimation.cpp.obj
[74/154] Building CXX object CMakeFiles\cgame.dir\cgame\dnanimation.cpp.obj
[75/154] Building CXX object CMakeFiles\cgame.dir\cgame\dnweaponanim.cpp.obj
[76/154] Building CXX object CMakeFiles\cgame.dir\qcommon\dnplayermove.cpp.obj
[77/154] Building CXX object CMakeFiles\cgame.dir\qcommon\dntransport.cpp.obj
[78/154] Building CXX object CMakeFiles\cgame.dir\game\q_shared.cpp.obj
[79/154] Building CXX object CMakeFiles\ui.dir\ui\ui_main.cpp.obj
[80/154] Building CXX object CMakeFiles\cgame.dir\qcommon\dncon.cpp.obj
[81/154] Building CXX object CMakeFiles\ui.dir\ui\ui_duke_menus.cpp.obj
[82/154] Building CXX object CMakeFiles\cgame.dir\qcommon\dnmap.cpp.obj
[83/154] Building CXX object CMakeFiles\ui.dir\ui\ui_util.cpp.obj
[84/154] Building CXX object CMakeFiles\ui.dir\ui\ui_shared.cpp.obj
FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2448,1): error: expected '}'
 2448 | 
      | ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2439,48): note: to match this '{'
 2439 | static void Scroll_ListBox_AutoFunc( void *p ) {
      |                                                ^
1 error generated.
[85/154] Building CXX object CMakeFiles\ui.dir\game\q_shared.cpp.obj
[86/154] Building CXX object CMakeFiles\ui.dir\ui\ui_duke_menus2.cpp.obj
[87/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[88/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[89/154] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

