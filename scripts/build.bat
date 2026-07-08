@echo off
setlocal EnableDelayedExpansion

REM =========================================================
REM Resolve project root from this script location
REM =========================================================

set "SCRIPT_DIR=%~dp0"
for %%i in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fi"

REM =========================================================
REM Config
REM =========================================================

if "%BUILD_DIR%"=="" set "BUILD_DIR=%PROJECT_ROOT%\build"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Debug"
if "%JOBS%"=="" set "JOBS=%NUMBER_OF_PROCESSORS%"
REM Comma list of sanitizers, e.g. set SANITIZE=address ^&^& scripts\build.bat
REM MSVC only supports AddressSanitizer (no UndefinedBehaviorSanitizer).
if "%SANITIZE%"=="" set "SANITIZE="

echo.
echo Project root : %PROJECT_ROOT%
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

cmake "%PROJECT_ROOT%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
    %SANITIZE_ARGS%

if errorlevel 1 exit /b 1

REM =========================================================
REM Build
REM =========================================================

cmake --build . --config %BUILD_TYPE% -- /m:%JOBS%

if errorlevel 1 exit /b 1

echo.
echo Build complete!
echo Binaries are in:
echo %BUILD_DIR%\bin
echo.
