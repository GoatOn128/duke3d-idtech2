# Build digest

- Commit: `98d078b1ed93e3565a4f1c1ccc94551421c1d04d`
- Branch: `port/bfg-game-thread`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40895871531
- Generated: 2026-10-29 14:53:52 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.cpp(41),16`: definition of implicitly declared default constructor

  ```
  FAILED: [code=1] CMakeFiles/duke3d.dir/server/sv_gamethread.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj.d /FoCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.cpp
  ```


## Warnings

No warnings in the log.

## Log tail

```
[117/150] Building CXX object CMakeFiles\duke3d.dir\client\menu.cpp.obj
[118/150] Building CXX object CMakeFiles\duke3d.dir\client\qmenu.cpp.obj
[119/150] Building CXX object CMakeFiles\duke3d.dir\sound\sndwave.cpp.obj
[120/150] Building CXX object CMakeFiles\duke3d.dir\win32\net_wins.cpp.obj
[121/150] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
[122/150] Building CXX object CMakeFiles\duke3d.dir\sound\dnambience.cpp.obj
[123/150] Building CXX object CMakeFiles\duke3d.dir\win32\q_shwin.cpp.obj
[124/150] Building CXX object CMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj
[125/150] Building CXX object CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj
[126/150] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\ref_gl.dll
[127/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_game.cpp.obj
[128/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_ccmds.cpp.obj
[129/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_ents.cpp.obj
[130/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_init.cpp.obj
[131/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_send.cpp.obj
[132/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_user.cpp.obj
[133/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_world.cpp.obj
[134/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/server/sv_gamethread.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj.d /FoCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.cpp(41,16): error: definition of implicitly declared default constructor
   41 | DN_GameThread::DN_GameThread()
      |                ^
1 error generated.
[135/150] Building CXX object CMakeFiles\duke3d.dir\win32\vid_menu.cpp.obj
[136/150] Building CXX object CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj
[137/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_main.cpp.obj
[138/150] Building CXX object CMakeFiles\duke3d.dir\sdl\sdl_sys.cpp.obj
[139/150] Building CXX object CMakeFiles\duke3d.dir\sdl\sdl_vid.cpp.obj
ninja: build stopped: subcommand failed.
```

