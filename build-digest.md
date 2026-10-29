# Build digest

- Commit: `f12b5ed830734f55fa245d89cc521f21df17682a`
- Branch: `refgl-gl33`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40798058691
- Generated: 2026-10-29 01:04:39 UTC

Errors: **2**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 2 of 2 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\glw_imp.cpp(121),10`: use of undeclared identifier 'GLimp_InitGL'

  ```
  FAILED: [code=1] CMakeFiles/ref_gl.dir/win32/glw_imp.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\win32\glw_imp.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\win32\glw_imp.cpp.obj.d /FoCMakeFiles\ref_gl.dir\win32\glw_imp.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\glw_imp.cpp
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\glw_imp.cpp(460),14`: use of undeclared identifier 'wglSwapBuffers'; did you mean 'SwapBuffers'?

## Warnings

No warnings in the log.

## Log tail

```
[1/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_draw.cpp.obj
[2/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_mesh.cpp.obj
[3/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_light.cpp.obj
[4/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_rmain.cpp.obj
[5/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj
[6/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_image.cpp.obj
[7/144] Building CXX object CMakeFiles\ref_gl.dir\game\q_shared.cpp.obj
[8/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_rmisc.cpp.obj
[9/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_shaders.cpp.obj
[10/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_warp.cpp.obj
[11/144] Building CXX object CMakeFiles\ref_gl.dir\win32\glw_imp.cpp.obj
FAILED: [code=1] CMakeFiles/ref_gl.dir/win32/glw_imp.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\win32\glw_imp.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\win32\glw_imp.cpp.obj.d /FoCMakeFiles\ref_gl.dir\win32\glw_imp.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\glw_imp.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\glw_imp.cpp(121,10): error: use of undeclared identifier 'GLimp_InitGL'
  121 |     if (!GLimp_InitGL())
      |          ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\glw_imp.cpp(460,14): error: use of undeclared identifier 'wglSwapBuffers'; did you mean 'SwapBuffers'?
  460 |         if (!wglSwapBuffers(glw_state.hDC))
      |              ^~~~~~~~~~~~~~
      |              SwapBuffers
C:\Program Files (x86)\Windows Kits\10\\include\10.0.26100.0\\um\wingdi.h(6346,24): note: 'SwapBuffers' declared here
 6346 | WINGDIAPI BOOL  WINAPI SwapBuffers(HDC);
      |                        ^
2 errors generated.
[12/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_rsurf.cpp.obj
[13/144] Building CXX object CMakeFiles\ref_gl.dir\qcommon\dntransport.cpp.obj
[14/144] Building CXX object CMakeFiles\ref_gl.dir\win32\q_shwin.cpp.obj
[15/144] Building CXX object CMakeFiles\ref_gl.dir\qcommon\dnmap.cpp.obj
[16/144] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

