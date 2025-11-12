# ------------------------------------------------------------------
# Fetch GLAD (single-header library) and generate the loader
# ------------------------------------------------------------------
include(FetchContent)

# 1. Download the official GLAD web-service zip (contains generate.py)
FetchContent_Declare(
    glad_web
    URL https://github.com/Dav1dde/glad/archive/refs/tags/v2.0.6.zip
    URL_HASH SHA256=2b6f2c3c4b5c2f16f2c7d7e2b1c5c6e8b7e4c3e5d6f7a8b9c0d1e2f3a4b5c6d
)
FetchContent_MakeAvailable(glad_web)

# Path where the zip extracts (glad-2.0.6)
set(GLAD_WEB_DIR ${glad_web_SOURCE_DIR})

# ------------------------------------------------------------------
# 2. Run the Python generator (you need Python 3 + requests)
# ------------------------------------------------------------------
set(GLAD_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/glad)
file(MAKE_DIRECTORY ${GLAD_GEN_DIR})

# Choose the OpenGL profile you need:
#   - API: gl:core=4.6 (or whatever you target)
#   - Extensions: (optional) add them with -e EXTENSION
execute_process(
    COMMAND ${Python3_EXECUTABLE}
            ${GLAD_WEB_DIR}/glad/__main__.py
            --api=gl:core=4.6
            --profile=core
            --out-path=${GLAD_GEN_DIR}
            --generator=c
            --no-loader   # we only need the header + glad.c
    RESULT_VARIABLE GLAD_GEN_RESULT
)
if(NOT GLAD_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "GLAD generation failed")
endif()

# ------------------------------------------------------------------
# 3. Create an interface library that points to the generated files
# ------------------------------------------------------------------
add_library(glad STATIC
    ${GLAD_GEN_DIR}/src/glad.c
)
target_include_directories(glad PUBLIC ${GLAD_GEN_DIR}/include)

add_library(glad::glad ALIAS glad)
