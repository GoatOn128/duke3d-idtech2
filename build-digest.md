# Build digest

- Commit: `f70c34297b2e4e955ed6f8f64658b77ed75c8802`
- Branch: `feat/ui-menu-parser`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40974497826
- Generated: 2026-10-29 23:39:06 UTC

Errors: **123**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 60 of 123 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(382),3`: a type specifier is required for all declarations

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(382),13`: use of undeclared identifier 'script'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(383),2`: extraneous closing brace ('}')
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(384),1`: extraneous closing brace ('}')
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2430),49`: expected '}'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2430),49`: expected '}'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2430),49`: expected '}'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(427),52`: function definition is not allowed here

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/qcommon/dnmenuparser.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj -clang:-MFCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj.d /FoCMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(442),82`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(455),88`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(474),67`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(482),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(493),67`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(503),71`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(518),75`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(529),68`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(538),74`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(554),75`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(565),75`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(581),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(593),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(605),77`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(617),74`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(629),77`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(658),67`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(666),68`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(674),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(680),76`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(691),70`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(697),74`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(704),79`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(710),67`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(720),75`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(733),76`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(745),69`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(754),74`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(769),70`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(800),69`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(807),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(814),70`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(826),72`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(834),68`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(841),72`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(848),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(855),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(862),72`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(869),72`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(876),71`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(883),72`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(896),72`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(910),74`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(923),75`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(930),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(940),72`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(947),74`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(963),70`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(970),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(977),73`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(984),72`: function definition is not allowed here
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(991),77`: function definition is not allowed here

## Warnings

No warnings in the log.

## Log tail

```
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1775,63): error: function definition is not allowed here
 1775 | qboolean dnMenuParser::Menu_Parse( int handle, dnMenu *menu ) {
      |                                                               ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1810,47): error: function definition is not allowed here
 1810 | qboolean dnMenuParser::Menu_New( int handle ) {
      |                                               ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1833,54): error: function definition is not allowed here
 1833 | void dnMenuParser::Menu_Load( const char *menuFile ) {
      |                                                      ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1856,54): error: function definition is not allowed here
 1856 | void dnMenuParser::LoadMenus( const char *listFile ) {
      |                                                      ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1880,2): error: expected '}'
 1880 | }
      |  ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(412,17): note: to match this '{'
  412 |     while ( 1 ) {
      |                 ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(1880,2): error: expected '}'
 1880 | }
      |  ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnmenuparser.cpp(397,72): note: to match this '{'
  397 | qboolean dnMenuParser::PC_Script_Parse( int handle, const char **out ) {
      |                                                                        ^
116 errors generated.
[88/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj
[89/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[90/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[91/155] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

