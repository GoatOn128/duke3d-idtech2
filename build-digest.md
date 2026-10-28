# Build digest

- Commit: `8ed7ba0d9d5580c09cc313b25de5ad0119f08df7`
- Branch: `vscode-llvm-setup`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40782840104
- Generated: 2026-10-28 22:50:16 UTC

Errors: **1**, warnings: **13**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 1 of 1 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sound\sndmusic.cpp(6),10`: '../mp3/dr_mp3.h' file not found

  ```
  FAILED: [code=1] CMakeFiles/duke3d.dir/sound/sndmusic.cpp.obj
  "C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj.d /FoCMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\sound\sndmusic.cpp
  ```


## Warnings

13 warning(s) in 7 kind(s):

| Kind | Count | Locations |
| --- | --- | --- |
| non-void function | 4 | D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_image.cpp(1288) D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\in_win.cpp(615) D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\net_wins.cpp(131) (+1 more) |
| equality comparison | 2 | D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_rsurf.cpp(528) D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_rsurf.cpp(717) |
| invalid suffix | 2 | D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\files.cpp(860) D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\files.cpp(865) |
| implicit conversion | 2 | D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_main.cpp(470) D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\vid_dll.cpp(631) |
| using floating | 1 | D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711) |
| enumeration values | 1 | D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp(1615) |
| add explicit | 1 | D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp(1726) |

### Warning messages by kind

- **non-void function**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_image.cpp(1288)`: non-void function does not return a value in all control paths [-Wreturn-type]
- **equality comparison**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_rsurf.cpp(528)`: equality comparison with extraneous parentheses [-Wparentheses-equality]
- **invalid suffix**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\qcommon\files.cpp(860)`: invalid suffix on literal; C++11 requires a space between literal and identifier [-Wreserved-user-defined-literal]
- **implicit conversion**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_main.cpp(470)`: implicit conversion between pointer-to-function and pointer-to-object is a Microsoft extension [-Wmicrosoft-cast]
- **using floating**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_ents.cpp(1711)`: using floating point absolute value function 'fabs' when argument is of integer type [-Wabsolute-value]
- **enumeration values**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp(1615)`: enumeration values 'ex_free' and 'ex_explosion' not handled in switch [-Wswitch]
- **add explicit**
  - `D:\a\duke3d-idtech2\duke3d-idtech2\code\client\cl_tent.cpp(1726)`: add explicit braces to avoid dangling else [-Wdangling-else]

## Log tail

```
[125/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_init.cpp.obj
[126/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_main.cpp.obj
D:\a\duke3d-idtech2\duke3d-idtech2\code\server\sv_main.cpp(470,69): warning: implicit conversion between pointer-to-function and pointer-to-object is a Microsoft extension [-Wmicrosoft-cast]
  470 |     Com_BeginRedirect(RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);
      |                                                                     ^~~~~~~~~~~~~~~~
1 warning generated.
[127/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_send.cpp.obj
[128/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_user.cpp.obj
[129/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_world.cpp.obj
[130/140] Building CXX object CMakeFiles\duke3d.dir\win32\sys_win.cpp.obj
[131/140] Building CXX object CMakeFiles\duke3d.dir\win32\vid_dll.cpp.obj
D:\a\duke3d-idtech2\duke3d-idtech2\code\win32\vid_dll.cpp(631,35): warning: implicit conversion between pointer-to-function and pointer-to-object is a Microsoft extension [-Wmicrosoft-cast]
  631 |     if (re.Init(global_hInstance, MainWndProc) == -1)
      |                                   ^~~~~~~~~~~
1 warning generated.
[132/140] Building CXX object CMakeFiles\duke3d.dir\client\x86.cpp.obj
[133/140] Building CXX object CMakeFiles\duke3d.dir\win32\vid_menu.cpp.obj
[134/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\dntransport.cpp.obj
[135/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnmap.cpp.obj
[136/140] Building CXX object CMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj
FAILED: [code=1] CMakeFiles/duke3d.dir/sound/sndmusic.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj -clang:-MFCMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj.d /FoCMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj /FdCMakeFiles\duke3d.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\sound\sndmusic.cpp
D:\a\duke3d-idtech2\duke3d-idtech2\code\sound\sndmusic.cpp(6,10): fatal error: '../mp3/dr_mp3.h' file not found
    6 | #include "../mp3/dr_mp3.h"
      |          ^~~~~~~~~~~~~~~~~
1 error generated.
[137/140] Building CXX object CMakeFiles\duke3d.dir\sound\dnambience.cpp.obj
[138/140] Building CXX object CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj
[139/140] Building CXX object CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj
ninja: build stopped: subcommand failed.
```

