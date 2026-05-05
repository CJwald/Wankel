

cmake_minimum_required(VERSION 3.16)
project(Sandbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# THIS IS THE CORRECT SYNTAX
# ".." = go up to Wankel root
# "engine" = name of the build folder CMake creates inside Sandbox/build/
add_subdirectory(.. Wankel)

file(GLOB SANDBOX_SOURCES "*.cpp")
add_executable(Sandbox ${SANDBOX_SOURCES})
#add_executable(Sandbox SandboxApp.cpp)

# Link against the Wankel library that was just built
target_link_libraries(Sandbox PRIVATE Wankel)

# Optional: match your engine's client define
target_compile_definitions(Sandbox PRIVATE WANKEL_CLIENT)

# Put binary in bin/
set_target_properties(Sandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy shaders to bin dir
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
     DESTINATION ${CMAKE_BINARY_DIR}/bin)

# Copy Mesh to bin dir
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Assets
     DESTINATION ${CMAKE_BINARY_DIR}/bin)
