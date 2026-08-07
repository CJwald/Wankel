#pragma once

#include <imgui.h>

namespace Wankel {

// Named color palette for the engine's ImGui theme - edit these values to retheme every panel at
// once. Low saturation throughout; lime green and purple are the only two accent hues (see
// Mechtrix/docs/IMGUIRefactor.md Phase 7 for the design brief this follows). A client app that wants
// a different look can build its own ImGuiPalette and pass it to ApplyImGuiTheme() after ImGuiLayer
// has attached.
struct ImGuiPalette {
    ImVec4 Background {0.10f, 0.10f, 0.11f, 1.00f};    // window/dockspace background
    ImVec4 Surface {0.14f, 0.14f, 0.15f, 1.00f};       // panels, frames, inputs at rest
    ImVec4 SurfaceHover {0.19f, 0.19f, 0.21f, 1.00f};  // hovered frames/buttons - brighter, same hue
    ImVec4 SurfaceActive {0.23f, 0.23f, 0.25f, 1.00f}; // pressed/active frames
    ImVec4 Border {0.24f, 0.24f, 0.26f, 0.50f};        // subtle, low-contrast separators

    ImVec4 Text {0.88f, 0.88f, 0.86f, 1.00f};
    ImVec4 TextDisabled {0.50f, 0.50f, 0.50f, 1.00f};

    ImVec4 Accent {0.60f, 0.80f, 0.25f, 1.00f}; // lime green - primary/interactive
    ImVec4 AccentHover {0.68f, 0.86f, 0.36f, 1.00f};
    ImVec4 AccentAlt {0.58f, 0.46f, 0.78f, 1.00f}; // purple - secondary/structural (tabs, docking)
    ImVec4 AccentAltHover {0.68f, 0.57f, 0.86f, 1.00f};
};

// Applies `palette` to ImGui::GetStyle() plus a handful of rounding/border style tweaks. Call after
// ImGui::StyleColorsDark(), which this builds on top of for any ImGuiCol_ entries the palette doesn't
// explicitly cover.
void ApplyImGuiTheme(const ImGuiPalette& palette = ImGuiPalette {});

} // namespace Wankel
