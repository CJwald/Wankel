cmake_minimum_required(VERSION 3.16)
project(WankelEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add external dependencies first
add_subdirectory(external)

# Collect all engine sources
file(GLOB_RECURSE WANKEL_SOURCES "src/Wankel/*.cpp")

add_library(Wankel STATIC ${WANKEL_SOURCES})

# Include directories
target_include_directories(Wankel PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# Link dependencies
target_link_libraries(Wankel 
	PUBLIC
	    spdlog::spdlog
		imgui
	    glm::glm #TODO: make Private
	PRIVATE
	    SDL3::SDL3
	    glad::glad
	    glfw
)

# Make glad headers available to Wankel (and thus to Sandbox)
target_include_directories(Wankel PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include"
)

#target_include_directories(Wankel PUBLIC
#    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
#)

