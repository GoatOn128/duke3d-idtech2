# Build digest

- Commit: `eeef5f3e6e7b868f1987db82c49fa54cb16aa266`
- Branch: `port/bfg-threads-jobs`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40811902237
- Generated: 2026-10-29 03:24:56 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnjobs.cpp(388),17`: use of undeclared identifier 'SDL_GetCPUCount'

  ```
  FAILED: [code=1] CMakeFiles/duke3d.dir/qcommon/dnjobs.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj.d /FoCMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnjobs.cpp
  ```


## Warnings

No warnings in the log.

## Log tail

```
[121/145] Building CXX object CMakeFiles\duke3d.dir\win32\q_shwin.cpp.obj
[122/145] Building CXX object CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj
[123/145] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
[124/145] Building CXX object CMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj
[125/145] Building CXX object CMakeFiles\duke3d.dir\server\sv_ccmds.cpp.obj
[126/145] Building CXX object CMakeFiles\duke3d.dir\server\sv_game.cpp.obj
[127/145] Building CXX object CMakeFiles\duke3d.dir\server\sv_ents.cpp.obj
[128/145] Building CXX object CMakeFiles\duke3d.dir\server\sv_init.cpp.obj
[129/145] Building CXX object CMakeFiles\duke3d.dir\server\sv_send.cpp.obj
[130/145] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\ref_gl.dll
[131/145] Building CXX object CMakeFiles\duke3d.dir\server\sv_main.cpp.obj
[132/145] Building CXX object CMakeFiles\duke3d.dir\server\sv_world.cpp.obj
[133/145] Building CXX object CMakeFiles\duke3d.dir\server\sv_user.cpp.obj
[134/145] Building CXX object CMakeFiles\duke3d.dir\win32\vid_menu.cpp.obj
[135/145] Building CXX object CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj
[136/145] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnmap.cpp.obj
[137/145] Building CXX object CMakeFiles\duke3d.dir\qcommon\dntransport.cpp.obj
[138/145] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnthreads.cpp.obj
[139/145] Building RC object CMakeFiles\duke3d.dir\win32\dnicon.rc.res
[140/145] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/qcommon/dnjobs.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj.d /FoCMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnjobs.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\dnjobs.cpp(388,17): error: use of undeclared identifier 'SDL_GetCPUCount'
  388 |         count = SDL_GetCPUCount() - 1;
      |                 ^
1 error generated.
[141/145] Building CXX object CMakeFiles\duke3d.dir\sdl\sdl_sys.cpp.obj
[142/145] Building CXX object CMakeFiles\duke3d.dir\sdl\sdl_vid.cpp.obj
[143/145] Building C object CMakeFiles\convertgrp.dir\tools\convertGRP\convert_grp.c.obj
ninja: build stopped: subcommand failed.
```

