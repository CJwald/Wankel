cmake_minimum_required(VERSION 3.16)
project(WankelEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------- Options -------------------
option(WANKEL_BUILD_CLIENT "Build client components" ON)
option(WANKEL_BUILD_SERVER "Build server components" ON)
option(WANKEL_BUILD_SANDBOX "Build sandbox test app" ON)

# ------------------- External Libraries -------------------
add_subdirectory(external)

# SDL2 is now a target: SDL2::SDL2 (or SDL2-static depending on build)
# We expose it via alias for consistency
if(TARGET SDL2::SDL2)
    set(SDL2_TARGET SDL2::SDL2)
elseif(TARGET SDL2-static)
    set(SDL2_TARGET SDL2-static)
else()
    message(FATAL_ERROR "SDL2 target not found!")
endif()

# ------------------- Engine Library -------------------
file(GLOB_RECURSIVE WANKEL_CORE
    src/Wankel/Core/*.cpp
    src/Wankel/Utils/*.cpp
    src/Wankel/Platform/*.cpp
)

if(WANKEL_BUILD_CLIENT)
    file(GLOB_RECURSIVE WANKEL_CLIENT src/Wankel/Client/**/*.cpp)
    list(APPEND WANKEL_CORE ${WANKEL_CLIENT})
endif()
if(WANKEL_BUILD_SERVER)
    file(GLOB_RECURSIVE WANKEL_SERVER src/Wankel/Server/**/*.cpp)
    list(APPEND WANKEL_CORE ${WANKEL_SERVER})
endif()

add_library(Wankel ${WANKEL_CORE})

target_include_directories(Wankel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Wankel PUBLIC
    ${SDL2_TARGET}
    glad
    glm::glm
    spdlog::spdlog
)

target_compile_definitions(Wankel PUBLIC
    $<$<CONFIG:Debug>:WANKEL_DEBUG>
    $<$<BOOL:${WANKEL_BUILD_CLIENT}>:WANKEL_CLIENT>
    $<$<BOOL:${WANKEL_BUILD_SERVER}>:WANKEL_SERVER>
)

# ------------------- Sandbox -------------------
if(WANKEL_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

# ------------------- Install -------------------
install(TARGETS Wankel
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)
install(DIRECTORY include/Wankel DESTINATION include)
