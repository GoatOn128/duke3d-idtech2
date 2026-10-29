# Build digest

- Commit: `08e62da96ee7a922b5c9f963da9e8ab19638803a`
- Branch: `feat/ui-oop-methods`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40960524971
- Generated: 2026-10-29 21:47:09 UTC

Errors: **8**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 8 of 8 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2534),15`: expected unqualified-id

  ```
  FAILED: [code=1] CMakeFiles/ui.dir/ui/ui_shared.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dui_EXPORTS  /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ui.dir\ui\ui_shared.cpp.obj -clang:-MFCMakeFiles\ui.dir\ui\ui_shared.cpp.obj.d /FoCMakeFiles\ui.dir\ui\ui_shared.cpp.obj /FdCMakeFiles\ui.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2540),15`: expected unqualified-id
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2554),15`: expected unqualified-id
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6132),10`: expected unqualified-id
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6138),2`: invalid use of 'this' outside of a non-static member function
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6139),2`: call to non-static member function without an object argument
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6140),2`: invalid use of member 'window' in static member function
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6142),9`: invalid use of 'this' outside of a non-static member function

## Warnings

No warnings in the log.

## Log tail

```
 2534 |                         scrollInfo.this = this;
      |                                    ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2540,15): error: expected unqualified-id
 2540 |                         scrollInfo.this = this;
      |                                    ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(2554,15): error: expected unqualified-id
 2554 |                         scrollInfo.this = this;
      |                                    ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6132,10): error: expected unqualified-id
 6132 |         dnMenu *this;
      |                 ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6138,2): error: invalid use of 'this' outside of a non-static member function
 6138 |         this = &Menus[menuCount];
      |         ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6139,2): error: call to non-static member function without an object argument
 6139 |         Init();
      |         ^~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6140,2): error: invalid use of member 'window' in static member function
 6140 |         window.name = name;
      |         ^~~~~~
D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui_shared.cpp(6142,9): error: invalid use of 'this' outside of a non-static member function
 6142 |         return this;
      |                ^
8 errors generated.
[85/154] Building CXX object CMakeFiles\ui.dir\game\q_shared.cpp.obj
[86/154] Building CXX object CMakeFiles\ui.dir\ui\ui_duke_menus2.cpp.obj
[87/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[88/154] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[89/154] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

