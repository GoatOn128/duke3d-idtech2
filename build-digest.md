# Build digest

- Commit: `753d42c05c452121e829b48d53138ae7f217f9b6`
- Branch: `feat/ui-menu-parser`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40974862044
- Generated: 2026-10-29 23:42:41 UTC

Errors: **7**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 7 of 7 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1209),9`: use of undeclared identifier 'PC_SourceError'

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/qcommon/dnmenuparser.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj -clang:-MFCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj.d /FoCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1215),10`: use of undeclared identifier 'PC_Float_Parse'; did you mean 'Float_Parse'?
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1215),26`: cannot initialize a parameter of type 'char **' with an lvalue of type 'int'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1216),10`: use of undeclared identifier 'PC_Float_Parse'; did you mean 'Float_Parse'?
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1216),26`: cannot initialize a parameter of type 'char **' with an lvalue of type 'int'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1217),10`: use of undeclared identifier 'PC_Color_Parse'; did you mean 'Color_Parse'?
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1217),26`: cannot initialize a parameter of type 'char **' with an lvalue of type 'int'

## Warnings

No warnings in the log.

## Log tail

```
      |          Float_Parse
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\../ui/ui_shared.h(506,10): note: 'Float_Parse' declared here
  506 | qboolean Float_Parse( char **p, float *f );
      |          ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1216,26): error: cannot initialize a parameter of type 'char **' with an lvalue of type 'int'
 1216 |          PC_Float_Parse( handle, &color.high ) &&
      |                          ^~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\../ui/ui_shared.h(506,30): note: passing argument to parameter 'p' here
  506 | qboolean Float_Parse( char **p, float *f );
      |                              ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1217,10): error: use of undeclared identifier 'PC_Color_Parse'; did you mean 'Color_Parse'?
 1217 |          PC_Color_Parse( handle, &color.color ) ) {
      |          ^~~~~~~~~~~~~~
      |          Color_Parse
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\../ui/ui_shared.h(507,10): note: 'Color_Parse' declared here
  507 | qboolean Color_Parse( char **p, vec4_t *c );
      |          ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1217,26): error: cannot initialize a parameter of type 'char **' with an lvalue of type 'int'
 1217 |          PC_Color_Parse( handle, &color.color ) ) {
      |                          ^~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\../ui/ui_shared.h(507,30): note: passing argument to parameter 'p' here
  507 | qboolean Color_Parse( char **p, vec4_t *c );
      |                              ^
7 errors generated.
[88/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_ents.cpp.obj
[89/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[90/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[91/155] Building CXX object CMakeFiles\ui.dir\ui\ui_shared.cpp.obj
[92/155] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

