@echo off
setlocal
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
if not exist "%MSYS2_ROOT%\usr\bin\bash.exe" set "MSYS2_ROOT=D:\msys64"
if not exist "%MSYS2_ROOT%\usr\bin\bash.exe" (
    echo MSYS2 was not found. Follow README.md or set MSYS2_ROOT to its folder.
    pause
    exit /b 1
)
set "MSYSTEM=UCRT64"
set "CHERE_INVOKING=1"
cd /d "%~dp0"
"%MSYS2_ROOT%\usr\bin\bash.exe" --login -c "bash ./build_windows.sh"
if errorlevel 1 pause
