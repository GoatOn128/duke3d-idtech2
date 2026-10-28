# Build digest

- Commit: `ffed99be73bee47c53d8a4a9cf39f78cabae18eb`
- Branch: `vscode-llvm-setup`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40783998262
- Generated: 2026-10-28 22:59:41 UTC

Errors: **1**, warnings: **1**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp(1736),1`: extraneous closing brace ('}')

  ```
  FAILED: [code=1] CMakeFiles/duke3d.dir/client/cl_tent.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj.d /FoCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp
  ```


## Warnings

1 warning(s) in 1 kind(s):

| Kind | Count | Locations |
| --- | --- | --- |
| using floating | 1 | D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711) |

### Warning messages by kind

- **using floating**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711)`: using floating point absolute value function 'fabs' when argument is of integer type [-Wabsolute-value]

## Log tail

```
      |         ^~~~
      |         std::abs
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711,9): note: include the header <cstdlib> or explicitly provide a declaration for 'std::abs'
1 warning generated.
[86/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_inv.cpp.obj
[87/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_fx.cpp.obj
[88/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_main.cpp.obj
[89/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_parse.cpp.obj
[90/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_newfx.cpp.obj
[91/140] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[92/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_pred.cpp.obj
[93/140] Building CXX object CMakeFiles\duke3d.dir\client\dnmenu.cpp.obj
[94/140] Building CXX object CMakeFiles\duke3d.dir\client\dnsavemenu.cpp.obj
[95/140] Building CXX object CMakeFiles\duke3d.dir\client\dnhud.cpp.obj
[96/140] Building CXX object CMakeFiles\duke3d.dir\client\dnintermission.cpp.obj
[97/140] Building CXX object CMakeFiles\duke3d.dir\client\dnintro.cpp.obj
[98/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_scrn.cpp.obj
[99/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_tent.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/client/cl_tent.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj.d /FoCMakeFiles\duke3d.dir\client\cl_tent.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp(1736,1): error: extraneous closing brace ('}')
 1736 | }
      | ^
1 error generated.
[100/140] Building CXX object CMakeFiles\duke3d.dir\client\cl_view.cpp.obj
[101/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\cmd.cpp.obj
[102/140] Building CXX object CMakeFiles\duke3d.dir\client\dncgame.cpp.obj
[103/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\cmodel.cpp.obj
[104/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\common.cpp.obj
ninja: build stopped: subcommand failed.
```

