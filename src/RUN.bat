@echo off
setlocal
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
if not exist "%MSYS2_ROOT%\ucrt64\bin\Qt6Core.dll" set "MSYS2_ROOT=D:\msys64"
if not exist "%MSYS2_ROOT%\ucrt64\bin\Qt6Core.dll" (
    echo Qt was not found. Follow README.md or set MSYS2_ROOT to your MSYS2 folder.
    pause
    exit /b 1
)
if not exist "%~dp0build\BarcodeStudio.exe" (
    echo Build the application first using BUILD_AND_RUN.bat.
    pause
    exit /b 1
)
set "PATH=%MSYS2_ROOT%\ucrt64\bin;%PATH%"
cd /d "%~dp0"
"%~dp0build\BarcodeStudio.exe"
if errorlevel 1 (
    echo The application could not start. Check the message above and README.md.
    pause
    exit /b 1
)
