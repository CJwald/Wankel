@echo off
setlocal EnableDelayedExpansion

REM Set BUILD_DIR (default to current directory + \build)
if "%BUILD_DIR%"=="" (
    set "BUILD_DIR=%CD%\build"
)

set "BINARY=%BUILD_DIR%\bin\Cube.exe"

REM Check if the binary exists
if not exist "%BINARY%" (
    echo Cube binary not found^^!
    echo Run .\scripts\build.bat first.
    exit /b 1
)

echo Launching Cube ...
"%BINARY%" %*
