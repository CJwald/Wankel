set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

# Prevent building GLFW's examples, tests, and docs
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)

add_subdirectory(SDL3 EXCLUDE_FROM_ALL)
add_subdirectory(glad EXCLUDE_FROM_ALL)
add_subdirectory(glm EXCLUDE_FROM_ALL)
add_subdirectory(spdlog EXCLUDE_FROM_ALL)
add_subdirectory(glfw EXCLUDE_FROM_ALL)
add_subdirectory(imgui EXCLUDE_FROM_ALL)

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "SDL3::SDL3 missing!")
endif()

