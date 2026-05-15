cmake_minimum_required(VERSION 3.16)
project(WankelEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Multi-config generators (Visual Studio)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER}
        ${CMAKE_BINARY_DIR}/lib)

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER}
        ${CMAKE_BINARY_DIR}/bin)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER}
        ${CMAKE_BINARY_DIR}/bin)
endforeach()

# Add external dependencies first
add_subdirectory(external)
include(cmake/imgui.cmake)

# Collect all engine sources
file(GLOB_RECURSE WANKEL_SOURCES
    "src/Wankel/*.cpp"
)

# Remove unsupported platform sources
if (WIN32)
    list(FILTER WANKEL_SOURCES EXCLUDE REGEX "Platform/Linux/")
elseif(UNIX)
    list(FILTER WANKEL_SOURCES EXCLUDE REGEX "Platform/Windows/")
endif()

# Wankel library
add_library(Wankel STATIC ${WANKEL_SOURCES})

# Copy Wankel internal shaders
add_custom_command(TARGET Wankel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:Wankel>/WankelShaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Wankel/Renderer/shaders
        $<TARGET_FILE_DIR:Wankel>/WankelShaders
)

source_group(
	TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
	FILES ${WANKEL_SOURCES}
)

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
	    glad::glad #TODO: make Private
	PRIVATE
	    SDL3::SDL3
	    glfw
)

target_compile_definitions(Wankel PUBLIC GLM_ENABLE_EXPERIMENTAL)

# Make glad headers available to Wankel (and thus to Sandbox)
target_include_directories(Wankel PUBLIC #PRIVATE # TODO: I WOULD LIKE THIS PRIVATE OR GONE? 
    "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/entt/src"
)



