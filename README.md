# Wankel
Game Engine designed for 3D Rendering and Physics. 

![Wankel Sandbox Screenshot](Documents/WankelScreenshot.png)


## REQUIREMENTS:
g++ 13+ for entt


## EXAMPLE SETUP:
### Initialize Submodules:
```bash
git submodule update --init --recursive
```
#### LINUX:
```bash
./scripts/build.sh
cd Sandbox
./build.sh
./run.sh
```


#### WINDOWS:
```bash
.\scripts\build.bat
cd Sandbox
.\build.bat
.\run.bat
```


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
- Controller Input
- Event system
- Window system
- Camera
- Camera Controller
- Simple Renderer
- ECS using Entt
- Sandbox example
- AABB Collision
- Second Order Dynamics Animation System
- Multi-mesh components


## TODO:
- Debug Visualization system (coord. frames etc.)
- Procedural Animation Layering
- Raycast/collision queries
- Sphere and capsule collision systems
- Text Rendering
- Basic UI/HUD Framework
- Asset/Material separation
- Audio System
- Static triangle mesh collision
- Lighting/Material pipeline
- State machine helpers
- Multiplayer Networking

