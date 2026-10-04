#pragma once

#include <imgui.h>

#include <cstdint>

// Showing GL textures (render targets, loaded images) through ImGui.
namespace Wankel::ImGuiTexture {

inline ImTextureID FromGL(uint32_t glTexture) {
    return (ImTextureID)(uintptr_t)glTexture;
}

// GL textures (loaded flipped, and render targets) have v=0 at the bottom; ImGui draws v=0 at the top.
inline ImVec2 UvTop() {
    return {0.0f, 1.0f};
}
inline ImVec2 UvBottom() {
    return {1.0f, 0.0f};
}

} // namespace Wankel::ImGuiTexture
