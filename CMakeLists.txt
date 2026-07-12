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

# ---- Sanitizers ----
# Usage: cmake -DWANKEL_ENABLE_ASAN=ON -DWANKEL_ENABLE_UBSAN=ON ...
# (or via scripts/build.sh: SANITIZE=address,undefined ./scripts/build.sh)
# Applied globally (before add_subdirectory(external)) so the sanitizer
# runtime is linked consistently across the whole build, including the
# vendored dependencies.
option(WANKEL_ENABLE_ASAN "Build with AddressSanitizer" OFF)
option(WANKEL_ENABLE_UBSAN "Build with UndefinedBehaviorSanitizer" OFF)

if(WANKEL_ENABLE_ASAN OR WANKEL_ENABLE_UBSAN)
    if(MSVC)
        if(WANKEL_ENABLE_UBSAN)
            message(WARNING "UndefinedBehaviorSanitizer is not supported by MSVC - ignoring WANKEL_ENABLE_UBSAN")
        endif()
        if(WANKEL_ENABLE_ASAN)
            add_compile_options(/fsanitize=address)
        endif()
    else()
        set(WANKEL_SANITIZE_LIST "")
        if(WANKEL_ENABLE_ASAN)
            list(APPEND WANKEL_SANITIZE_LIST "address")
        endif()
        if(WANKEL_ENABLE_UBSAN)
            list(APPEND WANKEL_SANITIZE_LIST "undefined")
        endif()
        list(JOIN WANKEL_SANITIZE_LIST "," WANKEL_SANITIZE_FLAGS)

        add_compile_options(-fsanitize=${WANKEL_SANITIZE_FLAGS} -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=${WANKEL_SANITIZE_FLAGS})
    endif()
endif()

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
	    SDL3::SDL3 #TODO: make Private
	PRIVATE
	    glfw
)

target_compile_definitions(Wankel PUBLIC GLM_ENABLE_EXPERIMENTAL)

# miniaudio's Linux backends (ALSA/PulseAudio/etc.) are loaded via dlopen at
# runtime, but it still needs these three linked at build time.
if(UNIX AND NOT APPLE)
    target_link_libraries(Wankel PRIVATE dl pthread m)
endif()

# Make glad headers available to Wankel (and thus to Sandbox)
target_include_directories(Wankel PUBLIC #PRIVATE # TODO: I WOULD LIKE THIS PRIVATE OR GONE?
    "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/entt/src"
)

# cgltf, stb (stb_truetype), and miniaudio are single-header libraries with
# no CMakeLists of their own, so just add their directories to the include
# path rather than add_subdirectory-ing them.
target_include_directories(Wankel PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/cgltf"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio"
)



