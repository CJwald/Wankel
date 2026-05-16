@echo off
setlocal EnableDelayedExpansion

REM =========================================================
REM Resolve sandbox root
REM =========================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR:~0,-1%"

REM =========================================================
REM Config
REM =========================================================

if "%BUILD_DIR%"=="" set "BUILD_DIR=%PROJECT_ROOT%\build"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Debug"

echo.
echo Sandbox root : %PROJECT_ROOT%
echo Build dir    : %BUILD_DIR%
echo Build type   : %BUILD_TYPE%
echo.

REM =========================================================
REM Create / enter build dir
REM =========================================================

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cd /d "%BUILD_DIR%"

REM =========================================================
REM Configure
REM =========================================================

cmake -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% "%PROJECT_ROOT%"

if errorlevel 1 exit /b 1

REM =========================================================
REM Build
REM =========================================================

cmake --build .

if errorlevel 1 exit /b 1

echo.
echo Build complete!
echo Binaries are in:
echo %BUILD_DIR%\bin
echo.