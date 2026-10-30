# Build digest

- Commit: `f1acdd4088597d76fe4759779d9c59637f8423d3`
- Branch: `feat/duke-listbox-feeders`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40981491513
- Generated: 2026-10-30 00:44:40 UTC

Errors: **3**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 3 of 3 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3668),9`: cannot initialize return object of type 'vec4_t *' (aka 'float (*)[4]') with an lvalue of type 'vec4_t' (aka 'float[4]')

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3796),90`: cannot initialize a parameter of type 'vec_t *' (aka 'float *') with an rvalue of type 'vec4_t *' (aka 'float (*)[4]')
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3804),60`: cannot initialize a parameter of type 'vec_t *' (aka 'float *') with an rvalue of type 'vec4_t *' (aka 'float (*)[4]')

## Warnings

No warnings in the log.

## Log tail

```
[76/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dntransport.cpp.obj
[77/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dnplayermove.cpp.obj
[78/155] Building CXX object CMakeFiles\cgame.dir\game\q_shared.cpp.obj
[79/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dncon.cpp.obj
[80/155] Building CXX object CMakeFiles\ui.dir\ui\ui_main.cpp.obj
[81/155] Building CXX object CMakeFiles\ui.dir\ui\ui_duke_menus.cpp.obj
[82/155] Building CXX object CMakeFiles\cgame.dir\qcommon\dnmap.cpp.obj
[83/155] Building CXX object CMakeFiles\ui.dir\ui\ui_duke_menus2.cpp.obj
[84/155] Building CXX object CMakeFiles\ui.dir\ui\ui_util.cpp.obj
[85/155] Building CXX object CMakeFiles\ui.dir\game\q_shared.cpp.obj
[86/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj
[87/155] Building CXX object CMakeFiles\ui.dir\ui\ui_shared.cpp.obj
FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3668,9): error: cannot initialize return object of type 'vec4_t *' (aka 'float (*)[4]') with an lvalue of type 'vec4_t' (aka 'float[4]')
 3668 |         return ( row == item->cursorPos ) ? rowLit : rowDim;
      |                ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3796,90): error: cannot initialize a parameter of type 'vec_t *' (aka 'float *') with an rvalue of type 'vec4_t *' (aka 'float (*)[4]')
 3796 |                                                         DC->drawText( x + 4 + listPtr->columnInfo[j].pos, y, item->font, item->textscale, Item_ListBox_RowColor( item, i ), text, 0, listPtr->columnInfo[j].maxChars, item->textStyle );
      |                                                                                                                                           ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3804,60): error: cannot initialize a parameter of type 'vec_t *' (aka 'float *') with an rvalue of type 'vec4_t *' (aka 'float (*)[4]')
 3804 |                                                 DC->drawText( x + 4, y, item->font, item->textscale, Item_ListBox_RowColor( item, i ), text, 0, 0, item->textStyle );
      |                                                                                                      ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
3 errors generated.
[88/155] Building CXX object CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj
[89/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[90/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[91/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_ents.cpp.obj
[92/155] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

