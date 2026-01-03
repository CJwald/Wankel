# FINAL — glad.sh output (gl.c in src/)
add_library(glad STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gl.c"
)

target_include_directories(glad PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_library(glad::glad ALIAS glad)
