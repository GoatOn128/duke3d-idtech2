# Build digest

- Commit: `23a75225c7a0d5b06fd4fefa1c78aa51b2ad4044`
- Branch: `vscode-llvm-setup`
- Result: configure exit: 1, build exit: 
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40781623882
- Generated: 2026-10-28 22:40:01 UTC

Errors: **0**, warnings: **0**.

## CMake errors

```
CMake Error at C:/Program Files/CMake/share/cmake-4.4/Modules/CMakeTestCCompiler.cmake:65 (message):
```

## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```
  The C compiler

    "C:/Program Files/LLVM/bin/clang-cl.exe"

  is not able to compile a simple test program.

  It fails with the following output:

    Change Dir: 'D:/a/duke3d-idtech2/duke3d-idtech2/code/out/build/x86/CMakeFiles/CMakeScratch/TryCompile-2nib2e'
    
    Run Build Command(s): C:/ProgramData/chocolatey/bin/ninja.exe -v cmTC_50c98
    [1/2] "C:\Program Files\LLVM\bin\clang-cl.exe"  /nologo -D_MBCS  /DWIN32 /D_WINDOWS  /Zi /Ob0 /Od /RTC1 -MDd -clang:-MD -clang:-MTCMakeFiles\cmTC_50c98.dir\testCCompiler.c.obj -clang:-MFCMakeFiles\cmTC_50c98.dir\testCCompiler.c.obj.d /FoCMakeFiles\cmTC_50c98.dir\testCCompiler.c.obj /FdCMakeFiles\cmTC_50c98.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\out\build\x86\CMakeFiles\CMakeScratch\TryCompile-2nib2e\testCCompiler.c
    [2/2] C:\Windows\system32\cmd.exe /C "cd . && "C:\Program Files\CMake\bin\cmake.exe" -E vs_link_exe --msvc-ver=1951 --intdir=CMakeFiles\cmTC_50c98.dir --rc="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\rc.exe" --mt="C:\Program Files\LLVM\bin\llvm-mt.exe" --manifests  -- "C:\Program Files\LLVM\bin\lld-link.exe" /nologo CMakeFiles\cmTC_50c98.dir\testCCompiler.c.obj  /out:cmTC_50c98.exe /implib:cmTC_50c98.lib /pdb:cmTC_50c98.pdb /version:0.0 /machine:x64  /debug /INCREMENTAL /subsystem:console  kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib && cd ."
    FAILED: [code=4294967295] cmTC_50c98.exe 
    C:\Windows\system32\cmd.exe /C "cd . && "C:\Program Files\CMake\bin\cmake.exe" -E vs_link_exe --msvc-ver=1951 --intdir=CMakeFiles\cmTC_50c98.dir --rc="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\rc.exe" --mt="C:\Program Files\LLVM\bin\llvm-mt.exe" --manifests  -- "C:\Program Files\LLVM\bin\lld-link.exe" /nologo CMakeFiles\cmTC_50c98.dir\testCCompiler.c.obj  /out:cmTC_50c98.exe /implib:cmTC_50c98.lib /pdb:cmTC_50c98.pdb /version:0.0 /machine:x64  /debug /INCREMENTAL /subsystem:console  kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib && cd ."
    LINK Pass 1: command "C:\Program Files\LLVM\bin\lld-link.exe /nologo CMakeFiles\cmTC_50c98.dir\testCCompiler.c.obj /out:cmTC_50c98.exe /implib:cmTC_50c98.lib /pdb:cmTC_50c98.pdb /version:0.0 /machine:x64 /debug /INCREMENTAL /subsystem:console kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST /MANIFESTFILE:CMakeFiles\cmTC_50c98.dir/intermediate.manifest CMakeFiles\cmTC_50c98.dir/manifest.res" failed (exit code 1) with the following output:
    lld-link: error: <root>: undefined symbol: mainCRTStartup

    ninja: build stopped: subcommand failed.
    
    



  CMake will not be able to correctly generate this project.
Call Stack (most recent call first):
  CMakeLists.txt:2 (project)


-- Configuring incomplete, errors occurred!
```

