add_executable(Sandbox main.cpp)
target_link_libraries(Sandbox PRIVATE Wankel)

set_target_properties(Sandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
