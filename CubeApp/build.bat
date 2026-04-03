@echo off
setlocal

REM ---- Create / enter build dir ----
if not exist build mkdir build
cd /d build

REM ---- Configure ----
cmake .. -DCMAKE_BUILD_TYPE=Debug

REM ---- Build (parallel) ----
cmake --build . --config Debug -- /m

echo Done. Run: build\bin\Debug\Cube.exe

endlocal
