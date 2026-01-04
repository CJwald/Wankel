cmake_minimum_required(VERSION 3.16)
project(WankelEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external)

file(GLOB_RECURSE WANKEL_SOURCES "src/Wankel/*.cpp")
add_library(Wankel STATIC ${WANKEL_SOURCES})

target_include_directories(Wankel PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(Wankel PUBLIC
    SDL3::SDL3
    glad::glad
    glm::glm
    spdlog::spdlog
    glfw
)

