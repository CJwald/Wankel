

cmake_minimum_required(VERSION 3.16)
project(Sandbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# THIS IS THE CORRECT SYNTAX
# ".." = go up to Wankel root
# "engine" = name of the build folder CMake creates inside Sandbox/build/
add_subdirectory(.. Wankel)

#file(GLOB SANDBOX_SOURCES "*.cpp")
file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

set(SANDBOX_APP
    src/SandboxApp.cpp
)

add_executable(Sandbox
    ${SANDBOX_APP}
    ${SANDBOX_SOURCES}
)
#add_executable(Sandbox SandboxApp.cpp)

# Link against the Wankel library that was just built
target_link_libraries(Sandbox PRIVATE Wankel)

# Optional: match your engine's client define
target_compile_definitions(Sandbox PRIVATE WANKEL_CLIENT)

# Put binary in bin/
set_target_properties(Sandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Assets
    ${CMAKE_BINARY_DIR}/bin/Assets
)

add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
    ${CMAKE_BINARY_DIR}/bin/shaders
)

# Copy shaders to bin dir
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
#     DESTINATION ${CMAKE_BINARY_DIR}/bin)

# Copy Mesh to bin dir
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/Assets
#     DESTINATION ${CMAKE_BINARY_DIR}/bin)
