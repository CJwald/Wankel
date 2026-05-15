@echo off
setlocal EnableDelayedExpansion

REM =========================================================
REM Resolve sandbox root
REM =========================================================

set "SCRIPT_DIR=%~dp0"
for %%i in ("%SCRIPT_DIR%") do set "PROJECT_ROOT=%%~fi"

REM =========================================================
REM Config
REM =========================================================

if "%BUILD_DIR%"=="" set "BUILD_DIR=%PROJECT_ROOT%\build"

set "BINARY=%BUILD_DIR%\bin\Sandbox.exe"

REM =========================================================
REM Check binary
REM =========================================================

if not exist "%BINARY%" (
    echo Sandbox binary not found!
    echo Run build.bat first.
    exit /b 1
)

echo Launching Sandbox...
echo.

cd /d "%BUILD_DIR%\bin"

"%BINARY%" %*
