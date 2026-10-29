# Build digest

- Commit: `1489a4d70e12bc4a26d0882e83304cc390555cc8`
- Branch: `refgl-gl33`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40798217805
- Generated: 2026-10-29 01:06:04 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\dnrender.cpp(249),17`: use of undeclared identifier 'gluTessVertex'

  ```
  FAILED: [code=1] CMakeFiles/ref_gl.dir/ref_gl/dnrender.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj.d /FoCMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\dnrender.cpp
  ```


## Warnings

No warnings in the log.

## Log tail

```
[15/144] Building CXX object CMakeFiles\ref_gl.dir\qcommon\dnmath.cpp.obj
[16/144] Building CXX object CMakeFiles\ref_gl.dir\win32\q_shwin.cpp.obj
[17/144] Building CXX object CMakeFiles\game.dir\game\dnmain.cpp.obj
[18/144] Building CXX object CMakeFiles\ref_gl.dir\qcommon\dnvisibility.cpp.obj
[19/144] Building CXX object CMakeFiles\game.dir\game\dnrandom.cpp.obj
[20/144] Building CXX object CMakeFiles\ref_gl.dir\qcommon\dnmap.cpp.obj
[21/144] Building CXX object CMakeFiles\game.dir\game\dnassets.cpp.obj
[22/144] Building CXX object CMakeFiles\game.dir\qcommon\dnenemies.cpp.obj
[23/144] Building CXX object CMakeFiles\game.dir\game\dnentity.cpp.obj
[24/144] Building CXX object CMakeFiles\game.dir\game\dnlighting.cpp.obj
[25/144] Building CXX object CMakeFiles\game.dir\game\dndrop.cpp.obj
[26/144] Building CXX object CMakeFiles\game.dir\game\dnlights.cpp.obj
[27/144] Building CXX object CMakeFiles\game.dir\game\dnspawn.cpp.obj
[28/144] Building CXX object CMakeFiles\game.dir\game\dnintermission.cpp.obj
[29/144] Building CXX object CMakeFiles\game.dir\game\dnmultiplayer.cpp.obj
[30/144] Building CXX object CMakeFiles\game.dir\game\dnplayer.cpp.obj
[31/144] Building CXX object CMakeFiles\game.dir\game\dnweapons.cpp.obj
[32/144] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj
FAILED: [code=1] CMakeFiles/ref_gl.dir/ref_gl/dnrender.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj.d /FoCMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\dnrender.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\dnrender.cpp(249,17): error: use of undeclared identifier 'gluTessVertex'
  249 |                 gluTessVertex(t, vertices[j], vertices[j]);
      |                 ^
1 error generated.
[33/144] Building CXX object CMakeFiles\game.dir\game\dndamage.cpp.obj
[34/144] Building CXX object CMakeFiles\game.dir\game\dnactors.cpp.obj
[35/144] Building CXX object CMakeFiles\game.dir\game\dnprojectiles.cpp.obj
[36/144] Building CXX object CMakeFiles\game.dir\game\dncombat.cpp.obj
[37/144] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

