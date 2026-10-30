# Build digest

- Commit: `8c38298bcd107dab344418977e63d9ac85a7cc8a`
- Branch: `refactor/menu-framework-classes`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40991762974
- Generated: 2026-10-30 02:27:53 UTC

Errors: **3**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

- `lld-link: error: undefined symbol: public: virtual void __thiscall dnTextItem::PaintType(void)`
- `lld-link: error: undefined symbol: public: virtual int __thiscall dnListBoxItem::HandleTypeKey(int, int)`
- `lld-link: error: undefined symbol: public: virtual int __thiscall dnBindItem::HandleTypeKey(int, int)`
## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```
[95/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_parse.cpp.obj
[96/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_pred.cpp.obj
[97/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_newfx.cpp.obj
[98/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_main.cpp.obj
[99/155] Linking CXX shared library D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.dll
FAILED: [code=4294967295] D:/a/duke3d-idtech2/duke3d-idtech2/basedn/uix86.dll uix86.lib 
C:\Windows\system32\cmd.exe /C "cd . && "C:\Program Files\CMake\bin\cmake.exe" -E vs_link_dll --msvc-ver=1951 --intdir=CMakeFiles\ui.dir --rc="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe" --mt="C:\Program Files\LLVM\bin\llvm-mt.exe" --manifests  -- "C:\Program Files\LLVM\bin\lld-link.exe" /nologo CMakeFiles\ui.dir\ui\ui_main.cpp.obj CMakeFiles\ui.dir\ui\ui_duke_menus.cpp.obj CMakeFiles\ui.dir\ui\ui_duke_menus2.cpp.obj CMakeFiles\ui.dir\ui\ui_shared.cpp.obj CMakeFiles\ui.dir\ui\ui_util.cpp.obj CMakeFiles\ui.dir\game\q_shared.cpp.obj CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj  /out:D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.dll /implib:uix86.lib /pdb:D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.pdb /dll /version:0.0 /machine:X86 /INCREMENTAL:NO /DEF:D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui.def  kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib && cd ."
LINK: command "C:\Program Files\LLVM\bin\lld-link.exe /nologo CMakeFiles\ui.dir\ui\ui_main.cpp.obj CMakeFiles\ui.dir\ui\ui_duke_menus.cpp.obj CMakeFiles\ui.dir\ui\ui_duke_menus2.cpp.obj CMakeFiles\ui.dir\ui\ui_shared.cpp.obj CMakeFiles\ui.dir\ui\ui_util.cpp.obj CMakeFiles\ui.dir\game\q_shared.cpp.obj CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj /out:D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.dll /implib:uix86.lib /pdb:D:\a\duke3d-idtech2\duke3d-idtech2\basedn\uix86.pdb /dll /version:0.0 /machine:X86 /INCREMENTAL:NO /DEF:D:\a\duke3d-idtech2\duke3d-idtech2\code\ui\ui.def kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST:EMBED,ID=2" failed (exit code 1) with the following output:
lld-link: error: undefined symbol: public: virtual void __thiscall dnTextItem::PaintType(void)

>>> referenced by CMakeFiles\ui.dir\ui\ui_shared.cpp.obj:(const dnTextItem::`vftable')



lld-link: error: undefined symbol: public: virtual int __thiscall dnListBoxItem::HandleTypeKey(int, int)

>>> referenced by CMakeFiles\ui.dir\ui\ui_shared.cpp.obj:(const dnListBoxItem::`vftable')



lld-link: error: undefined symbol: public: virtual int __thiscall dnBindItem::HandleTypeKey(int, int)

>>> referenced by CMakeFiles\ui.dir\ui\ui_shared.cpp.obj:(const dnBindItem::`vftable')

[100/155] Building CXX object CMakeFiles\duke3d.dir\client\dnhud.cpp.obj
[101/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_scrn.cpp.obj
[102/155] Building CXX object CMakeFiles\duke3d.dir\client\dncgame.cpp.obj
[103/155] Building CXX object CMakeFiles\duke3d.dir\client\dnui.cpp.obj
[104/155] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```

