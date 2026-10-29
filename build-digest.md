# Build digest

- Commit: `1b1ba18fd6cfb2ac879187d6eec22db81c61989c`
- Branch: `feat/ui-shared-port`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40931982996
- Generated: 2026-10-29 18:34:42 UTC

Errors: **1**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

- `lld-link: error: undefined symbol: void __cdecl Com_Printf(char *, ...)`
## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```
[84/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_cin.cpp.obj
[85/156] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\cgamex86.dll
[86/156] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\dngamex86.dll
[87/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_input.cpp.obj
[88/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_inv.cpp.obj
[89/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_ents.cpp.obj
[90/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_fx.cpp.obj
[91/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_parse.cpp.obj
[92/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_main.cpp.obj
[93/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_newfx.cpp.obj
[94/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_pred.cpp.obj
[95/156] Building CXX object CMakeFiles\ui.dir\ui\ui_shared.cpp.obj
[96/156] Building CXX object CMakeFiles\duke3d.dir\client\cl_scrn.cpp.obj
[97/156] Building CXX object CMakeFiles\duke3d.dir\client\dnhud.cpp.obj
[98/156] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.dll
FAILED: [code=4294967295] D:/a/duke3d-idtech2/duke3d-idtech2/basedn/uix86.dll uix86.lib 
C:\Windows\system32\cmd.exe /C "cd . && "C:\Program Files\CMake\bin\cmake.exe" -E vs_link_dll --msvc-ver=1951 --intdir=CMakeFiles\ui.dir --rc="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe" --mt="C:\Program Files\LLVM\bin\llvm-mt.exe" --manifests  -- "C:\Program Files\LLVM\bin\lld-link.exe" /nologo CMakeFiles\ui.dir\ui\ui_main.cpp.obj CMakeFiles\ui.dir\ui\ui_shared.cpp.obj CMakeFiles\ui.dir\ui\ui_util.cpp.obj CMakeFiles\ui.dir\game\q_shared.cpp.obj  /out:D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.dll /implib:uix86.lib /pdb:D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.pdb /dll /version:0.0 /machine:X86 /INCREMENTAL:NO /DEF:D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui.def  kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib && cd ."
LINK: command "C:\Program Files\LLVM\bin\lld-link.exe /nologo CMakeFiles\ui.dir\ui\ui_main.cpp.obj CMakeFiles\ui.dir\ui\ui_shared.cpp.obj CMakeFiles\ui.dir\ui\ui_util.cpp.obj CMakeFiles\ui.dir\game\q_shared.cpp.obj /out:D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.dll /implib:uix86.lib /pdb:D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.pdb /dll /version:0.0 /machine:X86 /INCREMENTAL:NO /DEF:D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui.def kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST:EMBED,ID=2" failed (exit code 1) with the following output:
lld-link: error: undefined symbol: void __cdecl Com_Printf(char *, ...)

>>> referenced by CMakeFiles\ui.dir\game\q_shared.cpp.obj:(void __cdecl Com_sprintf(char *, int, char *, ...))

>>> referenced by CMakeFiles\ui.dir\game\q_shared.cpp.obj:(void __cdecl Info_SetValueForKey(char *, char *, char *))

[99/156] Building CXX object CMakeFiles\duke3d.dir\client\dnmenu.cpp.obj
[100/156] Building CXX object CMakeFiles\duke3d.dir\client\dnsavemenu.cpp.obj
[101/156] Building CXX object CMakeFiles\duke3d.dir\client\dncgame.cpp.obj
[102/156] Building CXX object CMakeFiles\duke3d.dir\client\dnui.cpp.obj
[103/156] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

