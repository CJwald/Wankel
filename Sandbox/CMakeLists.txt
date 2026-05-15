

cmake_minimum_required(VERSION 3.16)
project(Sandbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add Wankel Engine
add_subdirectory(.. Wankel)

# Sandbox sources
file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Sandbox executable
#set(SANDBOX_APP src/SandboxApp.cpp)
add_executable(Sandbox
    ${SANDBOX_APP}
    ${SANDBOX_SOURCES}
)

# Link Wankel Engine
target_link_libraries(Sandbox PRIVATE Wankel)

# Compile definitions
target_compile_definitions(Sandbox PRIVATE WANKEL_CLIENT)

# Put binary in bin/
set_target_properties(Sandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
# Copy Wankel runtime shaders
add_custom_command(TARGET Sandbox POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_FILE_DIR:Wankel>/WankelShaders
        $<TARGET_FILE_DIR:Sandbox>/WankelShaders
)

# Copy shaders to bin dir
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
