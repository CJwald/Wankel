@echo off
setlocal enabledelayedexpansion

REM ---- CONFIG ----
if "%BUILD_DIR%"=="" set BUILD_DIR=%cd%\build
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug
if "%JOBS%"=="" set JOBS=%NUMBER_OF_PROCESSORS%

REM ---- Resolve script directory (project root) ----
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..

echo Project root : %PROJECT_ROOT%
echo Build dir    : %BUILD_DIR%
echo Build type   : %BUILD_TYPE%

REM ---- Create / enter build dir ----
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM ---- Configure ----
cmake "%PROJECT_ROOT%" ^
	-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

REM ---- Build ----
cmake --build . --config %BUILD_TYPE% -- /m:%JOBS%

echo Build complete! Binaries are in %BUILD_DIR%\bin

endlocal
