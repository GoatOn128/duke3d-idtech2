# Build digest

- Commit: `b94bf80ee6f08bfddedccbd2c41c642b241424c0`
- Branch: `refgl-gl33`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40797875475
- Generated: 2026-10-29 01:02:58 UTC

Errors: **2**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 2 of 2 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_model.cpp(1049),31`: use of undeclared identifier 'MAX_LBM_HEIGHT'

  ```
  FAILED: [code=1] CMakeFiles/ref_gl.dir/ref_gl/gl_model.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj.d /FoCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_model.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_model.cpp(1051),81`: use of undeclared identifier 'MAX_LBM_HEIGHT'

## Warnings

No warnings in the log.

## Log tail

```
-- Check for working C compiler: C:/Program Files/LLVM/bin/clang-cl.exe - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/LLVM/bin/clang-cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Asset preparation skipped: needs 3drealms/duke3d.grp.
-- Configuring done (6.8s)
-- Generating done (0.1s)
-- Build files have been written to: D:/a/duke3d-idtech2/duke3d-idtech2/code/out/build/x86
=== build: windows-clang-x86 preset ===
[x86] VS environment ready: llvm.org clang-cl + lld-link with MSVC headers and libraries.
[1/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj
FAILED: [code=1] CMakeFiles/ref_gl.dir/ref_gl/gl_model.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj.d /FoCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_model.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_model.cpp(1049,31): error: use of undeclared identifier 'MAX_LBM_HEIGHT'
 1049 |     if (pheader->skinheight > MAX_LBM_HEIGHT)
      |                               ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_model.cpp(1051,81): error: use of undeclared identifier 'MAX_LBM_HEIGHT'
 1051 |         ri.Sys_Error(ERR_DROP, "model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT);
      |                                                                                 ^
2 errors generated.
[2/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_draw.cpp.obj
[3/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_mesh.cpp.obj
[4/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_rmain.cpp.obj
[5/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_light.cpp.obj
[6/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_image.cpp.obj
ninja: build stopped: subcommand failed.
```

