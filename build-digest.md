# Build digest

- Commit: `97a01fab19cf0f095e1c19f81ac79d7e363babb7`
- Branch: `port/bfg-game-thread`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40895367183
- Generated: 2026-10-29 14:50:46 UTC

Errors: **2**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 2 of 2 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.h(33),10`: 'dnthreads.h' file not found

  ```
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\server\sv_main.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\server\sv_main.cpp.obj.d /FoCMakeFiles\duke3d.dir\server\sv_main.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_main.cpp
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_main.cpp:22:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.h(33),10`: 'dnthreads.h' file not found

  ```
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj.d /FoCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.cpp
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.cpp:30:
  ```


## Warnings

No warnings in the log.

## Log tail

```
[122/150] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
[123/150] Building CXX object CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj
[124/150] Building CXX object CMakeFiles\duke3d.dir\win32\q_shwin.cpp.obj
[125/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_ccmds.cpp.obj
[126/150] Building CXX object CMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj
[127/150] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\ref_gl.dll
[128/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_ents.cpp.obj
[129/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_game.cpp.obj
[130/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_init.cpp.obj
[131/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_main.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/server/sv_main.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\server\sv_main.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\server\sv_main.cpp.obj.d /FoCMakeFiles\duke3d.dir\server\sv_main.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_main.cpp
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_main.cpp:22:
D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.h(33,10): fatal error: 'dnthreads.h' file not found
   33 | #include "dnthreads.h"
      |          ^~~~~~~~~~~~~
1 error generated.
[132/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_send.cpp.obj
[133/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_user.cpp.obj
[134/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_world.cpp.obj
[135/150] Building CXX object CMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/server/sv_gamethread.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj.d /FoCMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.cpp
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.cpp:30:
D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_gamethread.h(33,10): fatal error: 'dnthreads.h' file not found
   33 | #include "dnthreads.h"
      |          ^~~~~~~~~~~~~
1 error generated.
[136/150] Building CXX object CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj
ninja: build stopped: subcommand failed.
```

