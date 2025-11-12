set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory(SDL3 EXCLUDE_FROM_ALL)
add_subdirectory(glad/cmake EXCLUDE_FROM_ALL)
add_subdirectory(glm EXCLUDE_FROM_ALL)
add_subdirectory(spdlog EXCLUDE_FROM_ALL)

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "SDL3::SDL3 missing!")
endif()

message(STATUS "WANKEL EXTERNAL: SDL3 + GLAD + GLM + SPDLOG — CLEAN AND WORKING")
