# Build digest

- Commit: `9420eaba2c2058c8eba05e0ed3bbdd9a9b364c54`
- Branch: `feat/ui-module-cutover`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40944785289
- Generated: 2026-10-29 19:59:58 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

- `lld-link: error: undefined symbol: void __cdecl VID_MenuInit(void)`
## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```
[133/154] Building CXX object CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj
[134/154] Building CXX object CMakeFiles\duke3d.dir\server\sv_game.cpp.obj
[135/154] Building CXX object CMakeFiles\duke3d.dir\server\sv_init.cpp.obj
[136/154] Building CXX object CMakeFiles\duke3d.dir\server\sv_send.cpp.obj
[137/154] Building CXX object CMakeFiles\duke3d.dir\server\sv_user.cpp.obj
[138/154] Building CXX object CMakeFiles\duke3d.dir\server\sv_world.cpp.obj
[139/154] Building CXX object CMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj
[140/154] Building CXX object CMakeFiles\duke3d.dir\server\sv_main.cpp.obj
[141/154] Building CXX object CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj
[142/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnmap.cpp.obj
[143/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnerrortrap.cpp.obj
[144/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnthreads.cpp.obj
[145/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\dntransport.cpp.obj
[146/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\dneditfield.cpp.obj
[147/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnconsolehistory.cpp.obj
[148/154] Building CXX object CMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj
[149/154] Building CXX object CMakeFiles\duke3d.dir\sdl\sdl_sys.cpp.obj
[150/154] Building RC object CMakeFiles\duke3d.dir\win32\dnicon.rc.res
[151/154] Building CXX object CMakeFiles\duke3d.dir\sdl\sdl_vid.cpp.obj
[152/154] Building C object CMakeFiles\convertgrp.dir\tools\convertGRP\convert_grp.c.obj
[153/154] Linking CXX executable D:\a\duke3d-idtech2\duke3d-idtech2\duke3d.exe; Shipping soft_oal.dll as OpenAL32.dll; Shipping SDL3.dll
FAILED: [code=4294967295] D:/a/duke3d-idtech2/duke3d-idtech2/duke3d.exe 
C:\Windows\system32\cmd.exe /C "cd . && "C:\Program Files\CMake\bin\cmake.exe" -E vs_link_exe --msvc-ver=1951 --intdir=CMakeFiles\duke3d.dir --rc="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe" --mt="C:\Program Files\LLVM\bin\llvm-mt.exe" --manifests  -- "C:\Program Files\LLVM\bin\lld-link.exe" /nologo CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj CMakeFiles\duke3d.dir\client\cl_ents.cpp.obj CMakeFiles\duke3d.dir\client\cl_fx.cpp.obj CMakeFiles\duke3d.dir\client\cl_input.cpp.obj CMakeFiles\duke3d.dir\client\cl_inv.cpp.obj CMakeFiles\duke3d.dir\client\cl_main.cpp.obj CMakeFiles\duke3d.dir\client\cl_newfx.cpp.obj CMakeFiles\duke3d.dir\client\cl_parse.cpp.obj CMakeFiles\duke3d.dir\client\cl_pred.cpp.obj CMakeFiles\duke3d.dir\client\dncgame.cpp.obj CMakeFiles\duke3d.dir\client\dnui.cpp.obj CMakeFiles\duke3d.dir\client\cl_scrn.cpp.obj CMakeFiles\duke3d.dir\client\dnhud.cpp.obj CMakeFiles\duke3d.dir\client\dnmenu.cpp.obj CMakeFiles\duke3d.dir\client\dnintro.cpp.obj CMakeFiles\duke3d.dir\client\dnintermission.cpp.obj CMakeFiles\duke3d.dir\client\cl_tent.cpp.obj CMakeFiles\duke3d.dir\client\cl_view.cpp.obj CMakeFiles\duke3d.dir\qcommon\cmd.cpp.obj CMakeFiles\duke3d.dir\qcommon\cmodel.cpp.obj CMakeFiles\duke3d.dir\qcommon\common.cpp.obj CMakeFiles\duke3d.dir\win32\dn_syscon.cpp.obj CMakeFiles\duke3d.dir\client\console.cpp.obj CMakeFiles\duke3d.dir\client\dndebuggraph.cpp.obj CMakeFiles\duke3d.dir\qcommon\crc.cpp.obj CMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj CMakeFiles\duke3d.dir\qcommon\files.cpp.obj CMakeFiles\duke3d.dir\sdl\sdl_input.cpp.obj CMakeFiles\duke3d.dir\client\keys.cpp.obj CMakeFiles\duke3d.dir\game\m_flash.cpp.obj CMakeFiles\duke3d.dir\qcommon\md4.cpp.obj CMakeFiles\duke3d.dir\qcommon\net_chan.cpp.obj CMakeFiles\duke3d.dir\win32\net_wins.cpp.obj CMakeFiles\duke3d.dir\qcommon\pmove.cpp.obj CMakeFiles\duke3d.dir\game\q_shared.cpp.obj CMakeFiles\duke3d.dir\win32\q_shwin.cpp.obj CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj CMakeFiles\duke3d.dir\sound\dnambience.cpp.obj CMakeFiles\duke3d.dir\sound\sndwave.cpp.obj CMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj CMakeFiles\duke3d.dir\server\sv_ccmds.cpp.obj CMakeFiles\duke3d.dir\server\sv_ents.cpp.obj CMakeFiles\duke3d.dir\server\sv_game.cpp.obj CMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj CMakeFiles\duke3d.dir\server\sv_init.cpp.obj CMakeFiles\duke3d.dir\server\sv_main.cpp.obj CMakeFiles\duke3d.dir\server\sv_send.cpp.obj CMakeFiles\duke3d.dir\server\sv_user.cpp.obj CMakeFiles\duke3d.dir\server\sv_world.cpp.obj CMakeFiles\duke3d.dir\sdl\sdl_sys.cpp.obj CMakeFiles\duke3d.dir\sdl\sdl_vid.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnmap.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnthreads.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnerrortrap.cpp.obj CMakeFiles\duke3d.dir\qcommon\dntransport.cpp.obj CMakeFiles\duke3d.dir\qcommon\dneditfield.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnconsolehistory.cpp.obj CMakeFiles\duke3d.dir\win32\dnicon.rc.res  /out:D:\a\duke3d-idtech2\duke3d-idtech2\duke3d.exe /implib:duke3d.lib /pdb:D:\a\duke3d-idtech2\duke3d-idtech2\duke3d.pdb /version:0.0 /machine:X86 /INCREMENTAL:NO /subsystem:windows /SAFESEH:NO  D:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\libs\Win32\OpenAL32.lib  D:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\lib\x86\SDL3.lib  wsock32.lib  user32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib && C:\Windows\system32\cmd.exe /C "cd /D D:\a\duke3d-idtech2\duke3d-idtech2\code\out\build\x86 && "C:\Program Files\CMake\bin\cmake.exe" -E copy_if_different D:/a/duke3d-idtech2/duke3d-idtech2/code/third-party/openal-soft/bin/Win32/soft_oal.dll D:/a/duke3d-idtech2/duke3d-idtech2/OpenAL32.dll && cd /D D:\a\duke3d-idtech2\duke3d-idtech2\code\out\build\x86 && "C:\Program Files\CMake\bin\cmake.exe" -E copy_if_different D:/a/duke3d-idtech2/duke3d-idtech2/code/third-party/sdl3/lib/x86/SDL3.dll D:/a/duke3d-idtech2/duke3d-idtech2/SDL3.dll""
LINK: command "C:\Program Files\LLVM\bin\lld-link.exe /nologo CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj CMakeFiles\duke3d.dir\client\cl_ents.cpp.obj CMakeFiles\duke3d.dir\client\cl_fx.cpp.obj CMakeFiles\duke3d.dir\client\cl_input.cpp.obj CMakeFiles\duke3d.dir\client\cl_inv.cpp.obj CMakeFiles\duke3d.dir\client\cl_main.cpp.obj CMakeFiles\duke3d.dir\client\cl_newfx.cpp.obj CMakeFiles\duke3d.dir\client\cl_parse.cpp.obj CMakeFiles\duke3d.dir\client\cl_pred.cpp.obj CMakeFiles\duke3d.dir\client\dncgame.cpp.obj CMakeFiles\duke3d.dir\client\dnui.cpp.obj CMakeFiles\duke3d.dir\client\cl_scrn.cpp.obj CMakeFiles\duke3d.dir\client\dnhud.cpp.obj CMakeFiles\duke3d.dir\client\dnmenu.cpp.obj CMakeFiles\duke3d.dir\client\dnintro.cpp.obj CMakeFiles\duke3d.dir\client\dnintermission.cpp.obj CMakeFiles\duke3d.dir\client\cl_tent.cpp.obj CMakeFiles\duke3d.dir\client\cl_view.cpp.obj CMakeFiles\duke3d.dir\qcommon\cmd.cpp.obj CMakeFiles\duke3d.dir\qcommon\cmodel.cpp.obj CMakeFiles\duke3d.dir\qcommon\common.cpp.obj CMakeFiles\duke3d.dir\win32\dn_syscon.cpp.obj CMakeFiles\duke3d.dir\client\console.cpp.obj CMakeFiles\duke3d.dir\client\dndebuggraph.cpp.obj CMakeFiles\duke3d.dir\qcommon\crc.cpp.obj CMakeFiles\duke3d.dir\qcommon\cvar.cpp.obj CMakeFiles\duke3d.dir\qcommon\files.cpp.obj CMakeFiles\duke3d.dir\sdl\sdl_input.cpp.obj CMakeFiles\duke3d.dir\client\keys.cpp.obj CMakeFiles\duke3d.dir\game\m_flash.cpp.obj CMakeFiles\duke3d.dir\qcommon\md4.cpp.obj CMakeFiles\duke3d.dir\qcommon\net_chan.cpp.obj CMakeFiles\duke3d.dir\win32\net_wins.cpp.obj CMakeFiles\duke3d.dir\qcommon\pmove.cpp.obj CMakeFiles\duke3d.dir\game\q_shared.cpp.obj CMakeFiles\duke3d.dir\win32\q_shwin.cpp.obj CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj CMakeFiles\duke3d.dir\sound\dnambience.cpp.obj CMakeFiles\duke3d.dir\sound\sndwave.cpp.obj CMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj CMakeFiles\duke3d.dir\server\sv_ccmds.cpp.obj CMakeFiles\duke3d.dir\server\sv_ents.cpp.obj CMakeFiles\duke3d.dir\server\sv_game.cpp.obj CMakeFiles\duke3d.dir\server\sv_gamethread.cpp.obj CMakeFiles\duke3d.dir\server\sv_init.cpp.obj CMakeFiles\duke3d.dir\server\sv_main.cpp.obj CMakeFiles\duke3d.dir\server\sv_send.cpp.obj CMakeFiles\duke3d.dir\server\sv_user.cpp.obj CMakeFiles\duke3d.dir\server\sv_world.cpp.obj CMakeFiles\duke3d.dir\sdl\sdl_sys.cpp.obj CMakeFiles\duke3d.dir\sdl\sdl_vid.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnmap.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnthreads.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnjobs.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnerrortrap.cpp.obj CMakeFiles\duke3d.dir\qcommon\dntransport.cpp.obj CMakeFiles\duke3d.dir\qcommon\dneditfield.cpp.obj CMakeFiles\duke3d.dir\qcommon\dnconsolehistory.cpp.obj CMakeFiles\duke3d.dir\win32\dnicon.rc.res /out:D:\a\duke3d-idtech2\duke3d-idtech2\duke3d.exe /implib:duke3d.lib /pdb:D:\a\duke3d-idtech2\duke3d-idtech2\duke3d.pdb /version:0.0 /machine:X86 /INCREMENTAL:NO /subsystem:windows /SAFESEH:NO D:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\openal-soft\libs\Win32\OpenAL32.lib D:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\lib\x86\SDL3.lib wsock32.lib user32.lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST:EMBED,ID=1" failed (exit code 1) with the following output:
lld-link: error: undefined symbol: void __cdecl VID_MenuInit(void)

>>> referenced by CMakeFiles\duke3d.dir\sdl\sdl_vid.cpp.obj:(int __cdecl VID_LoadRefresh(char *))

[154/154] Linking C executable convertgrp.exe
ninja: build stopped: subcommand failed.
```

