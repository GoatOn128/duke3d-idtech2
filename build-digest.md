# Build digest

- Commit: `5e7698b2ee9222aa64b408ad1736e28d05bf7a7f`
- Branch: `feat/ui-roq-cinematics`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/41098261730
- Generated: 2026-10-30 17:23:15 UTC

Errors: **6**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 6 of 6 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(332),29`: use of undeclared identifier 'qtrue'; did you mean 'true'?

  ```
  FAILED: [code=1] CMakeFiles/duke3d.dir/client/roq.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\client\roq.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\client\roq.cpp.obj.d /FoCMakeFiles\duke3d.dir\client\roq.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(343),33`: use of undeclared identifier 'qtrue'; did you mean 'true'?
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(373),41`: use of undeclared identifier 'qtrue'; did you mean 'true'?
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(381),37`: use of undeclared identifier 'qtrue'; did you mean 'true'?
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(393),21`: use of undeclared identifier 'qtrue'; did you mean 'true'?
- `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(502),31`: use of undeclared identifier 'qtrue'; did you mean 'true'?

## Warnings

No warnings in the log.

## Log tail

```
  332 |             roq->finished = qtrue;
      |                             ^~~~~
      |                             true
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(343,33): error: use of undeclared identifier 'qtrue'; did you mean 'true'?
  343 |                 roq->finished = qtrue;
      |                                 ^~~~~
      |                                 true
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(373,41): error: use of undeclared identifier 'qtrue'; did you mean 'true'?
  373 |                         roq->finished = qtrue;
      |                                         ^~~~~
      |                                         true
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(381,37): error: use of undeclared identifier 'qtrue'; did you mean 'true'?
  381 |                     roq->finished = qtrue;
      |                                     ^~~~~
      |                                     true
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(393,21): error: use of undeclared identifier 'qtrue'; did you mean 'true'?
  393 |     roq->finished = qtrue;
      |                     ^~~~~
      |                     true
D:\a\duke3d-idtech2\duke3d-idtech2\code\client\roq.cpp(502,31): error: use of undeclared identifier 'qtrue'; did you mean 'true'?
  502 |         roq->finishReported = qtrue;
      |                               ^~~~~
      |                               true
6 errors generated.
[106/156] Building CXX object CMakeFiles\duke3d.dir\client\dnintermission.cpp.obj
[107/156] Building CXX object CMakeFiles\duke3d.dir\client\dnui.cpp.obj
[108/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_view.cpp.obj
[109/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_tent.cpp.obj
[110/156] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

