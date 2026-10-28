@echo off
rem Activates the Visual Studio developer environment for the given arch
rem (x86 by default). The compilers and linkers still come from llvm.org:
rem clang-cl.exe and lld-link.exe must be on PATH (the LLVM installer has an
rem "Add LLVM to the system path" option).
rem
rem Visual Studio itself only provides headers, libraries, rc.exe and the
rem debugger. Usage: call tools\vsenv.cmd [x86^|x64]
rem
set "DN_VS_ARCH=%~1"
if "%DN_VS_ARCH%"=="" set "DN_VS_ARCH=x86"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo vswhere.exe not found. Install VS Build Tools with the "Desktop development with C++" workload.
  exit /b 1
)

set "DN_VS_PATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "DN_VS_PATH=%%i"
if not defined DN_VS_PATH (
  echo No Visual Studio with C++ tools found. Install VS Build Tools with the "Desktop development with C++" workload.
  exit /b 1
)

call "%DN_VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=%DN_VS_ARCH% -host_arch=x64 >nul
if errorlevel 1 (
  echo Failed to activate the %DN_VS_ARCH% VS environment.
  exit /b 1
)

where clang-cl >nul 2>nul
if errorlevel 1 (
  echo clang-cl.exe not on PATH. Install LLVM from https://llvm.org/ and enable "Add LLVM to the system path".
  exit /b 1
)

echo [%DN_VS_ARCH%] VS environment ready: llvm.org clang-cl + lld-link with MSVC headers and libraries.
exit /b 0
