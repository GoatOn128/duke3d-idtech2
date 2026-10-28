# Build digest

- Commit: `be84827506b6860541068cbe6363e5e1059296f1`
- Branch: `vscode-llvm-setup`
- Result: configure exit: 0, build exit: 0
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40785485246
- Generated: 2026-10-28 23:11:50 UTC

Errors: **0**, warnings: **1**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

No errors in the log.

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
[111/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\md4.cpp.obj
[112/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\net_chan.cpp.obj
[113/140] Building CXX object CMakeFiles\duke3d.dir\win32\conproc.cpp.obj
[114/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\pmove.cpp.obj
[115/140] Building CXX object CMakeFiles\duke3d.dir\game\q_shared.cpp.obj
[116/140] Building CXX object CMakeFiles\duke3d.dir\client\menu.cpp.obj
[117/140] Building CXX object CMakeFiles\duke3d.dir\win32\in_win.cpp.obj
[118/140] Building CXX object CMakeFiles\duke3d.dir\client\qmenu.cpp.obj
[119/140] Building CXX object CMakeFiles\duke3d.dir\sound\sndwave.cpp.obj
[120/140] Building CXX object CMakeFiles\duke3d.dir\win32\net_wins.cpp.obj
[121/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_ccmds.cpp.obj
[122/140] Building CXX object CMakeFiles\duke3d.dir\win32\q_shwin.cpp.obj
[123/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_game.cpp.obj
[124/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_ents.cpp.obj
[125/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_init.cpp.obj
[126/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_main.cpp.obj
[127/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_send.cpp.obj
[128/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_user.cpp.obj
[129/140] Building CXX object CMakeFiles\duke3d.dir\server\sv_world.cpp.obj
[130/140] Building CXX object CMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj
[131/140] Building CXX object CMakeFiles\duke3d.dir\sound\dnambience.cpp.obj
[132/140] Building CXX object CMakeFiles\duke3d.dir\win32\sys_win.cpp.obj
[133/140] Building CXX object CMakeFiles\duke3d.dir\win32\vid_menu.cpp.obj
[134/140] Building CXX object CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj
[135/140] Building CXX object CMakeFiles\duke3d.dir\client\x86.cpp.obj
[136/140] Building CXX object CMakeFiles\duke3d.dir\win32\vid_dll.cpp.obj
[137/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\dntransport.cpp.obj
[138/140] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnmap.cpp.obj
[139/140] Building CXX object CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj
[140/140] Linking CXX executable D:\a\duke3d-idtech2\duke3d-idtech2\duke3d.exe; Shipping soft_oal.dll as OpenAL32.dll
```

