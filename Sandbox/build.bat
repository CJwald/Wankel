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
REM Comma list of sanitizers, e.g. set SANITIZE=address ^&^& build.bat
REM MSVC only supports AddressSanitizer (no UndefinedBehaviorSanitizer).
if "%SANITIZE%"=="" set "SANITIZE="

echo.
echo Sandbox root : %PROJECT_ROOT%
echo Build dir    : %BUILD_DIR%
echo Build type   : %BUILD_TYPE%
if not "%SANITIZE%"=="" echo Sanitizers   : %SANITIZE%
echo.

REM =========================================================
REM Translate SANITIZE into CMake options
REM =========================================================

set "SANITIZE_ARGS="
echo %SANITIZE% | findstr /C:"address" >nul && set "SANITIZE_ARGS=-DWANKEL_ENABLE_ASAN=ON"
echo %SANITIZE% | findstr /C:"undefined" >nul && echo NOTE: UndefinedBehaviorSanitizer is not supported by MSVC, ignoring.

REM =========================================================
REM Create / enter build dir
REM =========================================================

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cd /d "%BUILD_DIR%"

REM =========================================================
REM Configure
REM =========================================================

cmake -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% %SANITIZE_ARGS% "%PROJECT_ROOT%"

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