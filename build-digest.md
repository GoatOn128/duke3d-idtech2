# Build digest

- Commit: `1fba32145beebf6274c1dcfc3e4089439aec04bc`
- Branch: `feat/ui-oop-methods`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40960223984
- Generated: 2026-10-29 21:44:41 UTC

Errors: **57**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 57 of 57 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(157),6`: no member named 'RunScript' in 'dnItem'

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(807),14`: out-of-line definition of 'SetScreenCoords' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(825),14`: out-of-line definition of 'UpdatePosition' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(848),14`: out-of-line definition of 'UpdatePosition' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(864),14`: out-of-line definition of 'PostParse' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(874),17`: out-of-line definition of 'ClearFocus' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(884),14`: no member named 'RunScript' in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1082),14`: out-of-line definition of 'ShowItemByName' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1103),14`: out-of-line definition of 'FadeItemByName' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1134),9`: no member named 'Activate' in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1142),14`: out-of-line definition of 'RunCloseScript' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1146),8`: no member named 'RunScript' in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1336),14`: out-of-line definition of 'TransitionItemByName' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1371),14`: out-of-line definition of 'OrbitItemByName' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1414),16`: no member named 'RunScript' in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1512),14`: out-of-line definition of 'RunScript' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1548),18`: out-of-line definition of 'EnableShowViaCvar' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1588),18`: out-of-line definition of 'SetFocus' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1877),14`: out-of-line definition of 'MouseEnter' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1921),14`: out-of-line definition of 'MouseLeave' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1931),17`: out-of-line definition of 'HitTest' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(1941),14`: out-of-line definition of 'SetMouseOver' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2386),42`: no member named 'SetNextCursorItem' in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2393),42`: no member named 'SetPrevCursorItem' in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2518),14`: out-of-line definition of 'StartCapture' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2534),15`: expected unqualified-id
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2540),15`: expected unqualified-id
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2554),15`: expected unqualified-id
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2566),14`: out-of-line definition of 'StopCapture' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2611),18`: out-of-line definition of 'HandleKey' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2680),14`: out-of-line definition of 'Action' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2685),17`: out-of-line definition of 'SetPrevCursorItem' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2703),4`: use of undeclared identifier 'HandleMouseMove'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2712),17`: out-of-line definition of 'SetNextCursorItem' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2736),4`: use of undeclared identifier 'HandleMouseMove'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2753),14`: out-of-line definition of 'CloseCinematics' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2772),14`: out-of-line definition of 'Activate' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2812),18`: no member named 'OverActiveItem' in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2817),14`: no member named 'HandleMouseMove' in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2818),14`: no member named 'HandleKey' in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2831),20`: out-of-line definition of 'CorrectedTextRect' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2842),14`: out-of-line definition of 'HandleKey' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3030),14`: out-of-line definition of 'SetTextExtents' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(3068),14`: out-of-line definition of 'TextColor' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4216),14`: out-of-line definition of 'Paint' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4417),17`: out-of-line definition of 'GetFocusedItem' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4438),14`: out-of-line definition of 'ScrollFeeder' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4513),14`: out-of-line definition of 'HandleMouseMove' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4582),14`: out-of-line definition of 'Paint' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(4627),14`: out-of-line definition of 'ValidateTypeData' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6131),17`: out-of-line definition of 'Alloc' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6132),10`: expected unqualified-id
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6138),7`: expression is not assignable
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6153),17`: out-of-line definition of 'AddItem' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6282),14`: out-of-line definition of 'CacheContents' does not match any declaration in 'dnItem'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6288),14`: out-of-line definition of 'CacheContents' does not match any declaration in 'dnMenu'
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6310),18`: out-of-line definition of 'OverActiveItem' does not match any declaration in 'dnMenu'

## Warnings

No warnings in the log.

## Log tail

```
 6153 | dnItem *dnMenu::AddItem( const char *name ) {
      |                 ^~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.h(286,7): note: dnMenu defined here
  286 | class dnMenu {
      |       ^~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6282,14): error: out-of-line definition of 'CacheContents' does not match any declaration in 'dnItem'
 6282 | void dnItem::CacheContents( void ) {
      |              ^~~~~~~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.h(246,7): note: dnItem defined here
  246 | class dnItem {
      |       ^~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6288,14): error: out-of-line definition of 'CacheContents' does not match any declaration in 'dnMenu'
 6288 | void dnMenu::CacheContents( void ) {
      |              ^~~~~~~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.h(286,7): note: dnMenu defined here
  286 | class dnMenu {
      |       ^~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6310,18): error: out-of-line definition of 'OverActiveItem' does not match any declaration in 'dnMenu'
 6310 | qboolean dnMenu::OverActiveItem( float x, float y ) {
      |                  ^~~~~~~~~~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.h(286,7): note: dnMenu defined here
  286 | class dnMenu {
      |       ^~~~~~
57 errors generated.
[87/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj
[88/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[89/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[90/154] Building CXX object CMakeFiles\duke3d.dir\client\cl_ents.cpp.obj
[91/154] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

