# Build digest

- Commit: `73d2460daa52a776f77dd9478b198ffb98eb8f2e`
- Branch: `feat/ui-shared-port`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40931518096
- Generated: 2026-10-29 18:31:52 UTC

Errors: **52**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 52 of 52 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_util.cpp(102),19`: C-style cast from 'const vec_t *' (aka 'const float *') to 'vec3_t' (aka 'float[3]') is not allowed

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_util.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_util.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_util.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_util.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_util.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(930),10`: assigning to 'char *' from 'const char *' discards qualifiers

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(941),9`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(941),98`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(956),10`: assigning to 'char *' from 'const char *' discards qualifiers
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(970),9`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(970),98`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(991),8`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(994),15`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(997),15`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1041),8`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1080),10`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1082),17`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1085),17`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1142),8`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1365),34`: call to 'abs' is ambiguous
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1366),34`: call to 'abs' is ambiguous
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1367),34`: call to 'abs' is ambiguous
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1368),34`: call to 'abs' is ambiguous
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1435),53`: use of undeclared identifier 'CHAN_LOCAL_SOUND'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1473),50`: use of undeclared identifier 'CHAN_LOCAL_SOUND'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1551),10`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1588),10`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1593),10`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1660),30`: use of undeclared identifier 'CHAN_LOCAL_SOUND'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1743),18`: cannot initialize a variable of type 'editFieldDef_t *' (aka 'editFieldDef_s *') with an lvalue of type 'void *'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2215),10`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2241),10`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2517),18`: cannot initialize a variable of type 'editFieldDef_t *' (aka 'editFieldDef_s *') with an lvalue of type 'void *'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2597),20`: cannot initialize a variable of type 'editFieldDef_t *' (aka 'editFieldDef_s *') with an lvalue of type 'void *'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3108),84`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3109),77`: use of undeclared identifier 'BLINK_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3114),88`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3248),18`: use of undeclared identifier 'SAVE_INFOSTRING_LENGTH'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3353),83`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3381),83`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3407),83`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3673),8`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3689),8`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3722),83`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3765),83`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4139),26`: ordered comparison between pointer and zero ('qhandle_t' (aka 'void *') and 'int')
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4147),25`: ordered comparison between pointer and zero ('qhandle_t' (aka 'void *') and 'int')
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4209),81`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4210),78`: use of undeclared identifier 'BLINK_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4215),85`: use of undeclared identifier 'PULSE_DIVISOR'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4519),8`: no matching function for call to 'Q_stricmp'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4636),28`: use of undeclared identifier 'SCREEN_WIDTH'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4636),42`: use of undeclared identifier 'SCREEN_HEIGHT'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4703),6`: ISO C++17 does not allow 'register' storage class specifier [-Wregister]
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4703),6`: ISO C++17 does not allow 'register' storage class specifier [-Wregister]
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(5730),7`: no matching function for call to 'Q_stricmp'

## Warnings

No warnings in the log.

## Log tail

```
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4519,8): error: no matching function for call to 'Q_stricmp'
 4519 |                 if ( Q_stricmp( Menus[i].window.name, p ) == 0 ) {
      |                      ^~~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\../game/q_shared.h(227,5): note: candidate function not viable: 1st argument ('const char *') would lose const qualifier
  227 | int Q_stricmp(char *s1, char *s2);
      |     ^         ~~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4636,28): error: use of undeclared identifier 'SCREEN_WIDTH'
 4636 |                 DC->drawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu->window.background );
      |                                          ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4636,42): error: use of undeclared identifier 'SCREEN_HEIGHT'
 4636 |                 DC->drawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu->window.background );
      |                                                        ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4703,6): error: ISO C++17 does not allow 'register' storage class specifier [-Wregister]
 4703 |         int register hash, i;
      |             ^~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4703,6): error: ISO C++17 does not allow 'register' storage class specifier [-Wregister]
 4703 |         int register hash, i;
      |             ^~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(5730,7): error: no matching function for call to 'Q_stricmp'
 5730 |         if ( Q_stricmp( menu->window.name, "main" ) == 0 ) {
      |              ^~~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\../game/q_shared.h(227,5): note: candidate function not viable: 1st argument ('const char *') would lose const qualifier
  227 | int Q_stricmp(char *s1, char *s2);
      |     ^         ~~~~~~~~
51 errors generated.
[85/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj
[86/156] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[87/156] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[88/156] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

