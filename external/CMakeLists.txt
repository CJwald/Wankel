# external/CMakeLists.txt

# --- SDL2 ---
# Build as static library (recommended for engine)
add_subdirectory(SDL2 EXCLUDE_FROM_ALL)

# Optional: force static
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

# --- Other deps ---
add_subdirectory(glad)
add_subdirectory(glm)
add_subdirectory(spdlog)
