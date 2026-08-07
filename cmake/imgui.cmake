# cmake/imgui.cmake

add_library(imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_demo.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
)

target_link_libraries(imgui PUBLIC glfw)

# ImWchar defaults to 16-bit (unsigned short), which can't hold codepoints above the Basic
# Multilingual Plane (U+FFFF) - silently truncates instead of erroring, so eg. an emoji-range icon
# glyph (see Wankel/Core/ImGui/Icons.h, docs/IMGUIRefactor.md Phase 8) would alias to the wrong
# codepoint. PUBLIC so every translation unit that includes imgui.h agrees on ImWchar's width.
target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32)