# Wankel
Game Engine


## REQUIREMENTS:
g++ 13+ for entt


## EXAMPLE SETUP:
git submodule update --init --recursive
./scripts/build.sh    (or .\scripts\build.bat for Windows)
cd Sandbox
./build.sh            (or .\build.bat for Windows)
./run.sh              (or .\run.sh for Windows)


## Sandbox App Controls:
### MNK:
- W - Forward
- A - Left
- S - Backward
- D - Right
- Q - Roll Left
- E - Roll Right
- SPACE - Up
- Left Ctrl - Down
- Mouse - Pitch & Yaw
- Exc - Unlock Mouse
- Left Shift - Boost/Sprint
- X - Switch look mode (FPS to Start, FLIGHT)

### Controller (PS4 Notation):
- L Stick - Forward/Left/Backward/Right
- R Stick - Pitch & Yaw
- L2 - Roll Left
- R2 - Roll Right
- Cross - Up
- Circle - Down
- L3 - Boost/Sprint
- R3 - Switch look mode (FPS to Start, FLIGHT)


## FEATURES:
- imgui
- logging
- application and entry point
- Linux / Windows split
- Mouse and Keyboard input
- Event system
- Window system
- Camera
- Camera Controller
- Simple Renderer
- ECS using Entt
- Sandbox example


## TODO:
Controller Input
Make some external libs private to Wankel 
Fix window system. It doesnt resize correctly / does not work on 4k screens
Refine sandbox example to use more Wankel and less including external libs

