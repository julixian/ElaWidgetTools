@echo off
setlocal EnableExtensions

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
set "BUILD_DIR=%ROOT_DIR%\out\build"
set "INSTALL_DIR=%ROOT_DIR%\Install"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] cmake was not found in PATH.
    exit /b 1
)

echo [INFO] Configuring Release build...
cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" ^
    -G "Visual Studio 18 2026" ^
    -A x64 ^
    -T "v145,version=14.50" ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%"
if errorlevel 1 exit /b %errorlevel%

echo [INFO] Building and installing Release...
cmake --build "%BUILD_DIR%" --config Release --target install --parallel
if errorlevel 1 exit /b %errorlevel%

echo [INFO] Done. Installed to "%INSTALL_DIR%".
endlocal
pause
