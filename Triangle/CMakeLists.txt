

cmake_minimum_required(VERSION 3.16)
project(Triangle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# THIS IS THE CORRECT SYNTAX
# ".." = go up to Wankel root
# "engine" = name of the build folder CMake creates inside Triangle/build/
add_subdirectory(.. Wankel)

add_executable(Triangle TriangleApp.cpp)

# Link against the Wankel library that was just built
target_link_libraries(Triangle PRIVATE Wankel)

# Optional: match your engine's client define
target_compile_definitions(Triangle PRIVATE WANKEL_CLIENT)

# Put binary in bin/
set_target_properties(Triangle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
