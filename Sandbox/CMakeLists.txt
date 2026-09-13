

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

# HeadlessServerApp/.Layer belong to the separate HeadlessServerDemo target below (HeadlessServerApp
# defines its own Wankel::CreateApplication(), which would duplicate-symbol-conflict with
# SandboxApp.cpp's if both were linked into Sandbox).
list(FILTER SANDBOX_SOURCES EXCLUDE REGEX "NetDemo/HeadlessServer")

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

# Networking hooks verification demos (see src/NetDemo/) - off by default so they never affect the
# normal flying-ship Sandbox build. Enable with -DWANKEL_NET_DEMO=ON.
option(WANKEL_NET_DEMO "Build the networking hooks verification demos" OFF)
if(WANKEL_NET_DEMO)
    target_compile_definitions(Sandbox PRIVATE WANKEL_NET_DEMO)

    # Separate executable (not a Sandbox flag) since it needs its own headless
    # Wankel::CreateApplication() - proves ApplicationSpecification::Headless works standalone, with
    # no window/renderer/audio, unlike NetDemoLayer's windowed single-process demo.
    add_executable(HeadlessServerDemo
        src/NetDemo/HeadlessServerApp.cpp
        src/NetDemo/HeadlessServerLayer.cpp
    )
    target_link_libraries(HeadlessServerDemo PRIVATE Wankel)
    target_compile_definitions(HeadlessServerDemo PRIVATE WANKEL_CLIENT)
    set_target_properties(HeadlessServerDemo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

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

# Copy Wankel runtime fonts
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_FILE_DIR:Wankel>/WankelFonts
        $<TARGET_FILE_DIR:Sandbox>/WankelFonts
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
