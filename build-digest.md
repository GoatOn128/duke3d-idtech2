# Build digest

- Commit: `20237482b1c477d02a839436edb5019f9f3ac289`
- Branch: `feat/duke-ui-dehardcode`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/41085407571
- Generated: 2026-10-30 15:58:06 UTC

Errors: **9**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 9 of 9 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(2303),35`: out-of-line definition of 'Config' does not match any declaration in 'dnMenuParser'

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/qcommon/dnmenuparser.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj -clang:-MFCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj.d /FoCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(154),27`: no member named 'Config' in 'dnMenuParser'

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(159),27`: no member named 'Config' in 'dnMenuParser'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1370),67`: no member named 'Config' in 'dnMenuParser'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1408),64`: no member named 'Config' in 'dnMenuParser'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1592),44`: no member named 'Config' in 'dnMenuParser'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1802),66`: no member named 'Config' in 'dnMenuParser'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2041),68`: no member named 'Config' in 'dnMenuParser'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3316),30`: no member named 'Config' in 'dnMenuParser'

## Warnings

No warnings in the log.

## Log tail

```
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(154,27): error: no member named 'Config' in 'dnMenuParser'
  154 |     int v = dnMenuParser::Config()->focusPulse;
      |             ~~~~~~~~~~~~~~^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(159,27): error: no member named 'Config' in 'dnMenuParser'
  159 |     int v = dnMenuParser::Config()->focusBlink;
      |             ~~~~~~~~~~~~~~^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1370,67): error: no member named 'Config' in 'dnMenuParser'
 1370 |                                 DC->startLocalSound( DC->Assets.itemFocusSound, dnMenuParser::Config()->soundChannel );
      |                                                                                 ~~~~~~~~~~~~~~^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1408,64): error: no member named 'Config' in 'dnMenuParser'
 1408 |                 DC->startLocalSound( DC->registerSound( val ), dnMenuParser::Config()->soundChannel );      // all sounds are not 3d
      |                                                                ~~~~~~~~~~~~~~^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1592,44): error: no member named 'Config' in 'dnMenuParser'
 1592 |                 DC->startLocalSound( *sfx, dnMenuParser::Config()->soundChannel );
      |                                            ~~~~~~~~~~~~~~^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1802,66): error: no member named 'Config' in 'dnMenuParser'
 1802 |                         DC->startLocalSound( DC->Assets.itemFocusSound, dnMenuParser::Config()->soundChannel );
      |                                                                         ~~~~~~~~~~~~~~^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2041,68): error: no member named 'Config' in 'dnMenuParser'
 2041 |                                         DC->startLocalSound( DC->Assets.itemFocusSound, dnMenuParser::Config()->soundChannel );
      |                                                                                         ~~~~~~~~~~~~~~^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3316,30): error: no member named 'Config' in 'dnMenuParser'
 3316 |                 int maxLen = dnMenuParser::Config()->saveInfoLength;
      |                              ~~~~~~~~~~~~~~^
8 errors generated.
[88/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj
[89/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[90/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[91/155] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

