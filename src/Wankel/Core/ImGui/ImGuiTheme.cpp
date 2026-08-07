#include "wkpch.h"
#include "ImGuiTheme.h"

namespace Wankel {

void ApplyImGuiTheme(const ImGuiPalette& palette) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = palette.Background;
    colors[ImGuiCol_ChildBg] = palette.Background;
    colors[ImGuiCol_PopupBg] = palette.Surface;
    colors[ImGuiCol_Border] = palette.Border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    colors[ImGuiCol_Text] = palette.Text;
    colors[ImGuiCol_TextDisabled] = palette.TextDisabled;
    colors[ImGuiCol_TextLink] = palette.Accent;
    colors[ImGuiCol_TextSelectedBg] = palette.SurfaceActive;

    colors[ImGuiCol_FrameBg] = palette.Surface;
    colors[ImGuiCol_FrameBgHovered] = palette.SurfaceHover;
    colors[ImGuiCol_FrameBgActive] = palette.SurfaceActive;
    colors[ImGuiCol_InputTextCursor] = palette.Accent;

    colors[ImGuiCol_TitleBg] = palette.Background;
    colors[ImGuiCol_TitleBgActive] = palette.Surface;
    colors[ImGuiCol_TitleBgCollapsed] = palette.Background;

    colors[ImGuiCol_MenuBarBg] = palette.Surface;

    colors[ImGuiCol_ScrollbarBg] = palette.Background;
    colors[ImGuiCol_ScrollbarGrab] = palette.Surface;
    colors[ImGuiCol_ScrollbarGrabHovered] = palette.SurfaceHover;
    colors[ImGuiCol_ScrollbarGrabActive] = palette.SurfaceActive;

    colors[ImGuiCol_CheckMark] = palette.Accent;
    colors[ImGuiCol_CheckboxSelectedBg] = palette.Surface;
    colors[ImGuiCol_SliderGrab] = palette.Accent;
    colors[ImGuiCol_SliderGrabActive] = palette.AccentHover;

    // Subtle buttons: same fill as a resting frame, not a filled accent block - only the hover/active
    // states brighten, matching the "buttons should be subtle, hovered items slightly brighter" brief.
    colors[ImGuiCol_Button] = palette.Surface;
    colors[ImGuiCol_ButtonHovered] = palette.SurfaceHover;
    colors[ImGuiCol_ButtonActive] = palette.SurfaceActive;

    colors[ImGuiCol_Header] = palette.Surface;
    colors[ImGuiCol_HeaderHovered] = palette.SurfaceHover;
    colors[ImGuiCol_HeaderActive] = palette.SurfaceActive;

    colors[ImGuiCol_Separator] = palette.Border;
    colors[ImGuiCol_SeparatorHovered] = palette.AccentAlt;
    colors[ImGuiCol_SeparatorActive] = palette.AccentAltHover;

    colors[ImGuiCol_ResizeGrip] = palette.AccentAlt;
    colors[ImGuiCol_ResizeGripHovered] = palette.AccentAltHover;
    colors[ImGuiCol_ResizeGripActive] = palette.AccentAltHover;

    colors[ImGuiCol_TabHovered] = palette.AccentAltHover;
    colors[ImGuiCol_Tab] = palette.Surface;
    colors[ImGuiCol_TabSelected] = palette.SurfaceActive;
    colors[ImGuiCol_TabSelectedOverline] = palette.AccentAlt;
    colors[ImGuiCol_TabDimmed] = palette.Background;
    colors[ImGuiCol_TabDimmedSelected] = palette.Surface;
    colors[ImGuiCol_TabDimmedSelectedOverline] = palette.AccentAlt;

    colors[ImGuiCol_DockingPreview] = palette.AccentAlt;
    colors[ImGuiCol_DockingEmptyBg] = palette.Background;

    colors[ImGuiCol_PlotLines] = palette.Accent;
    colors[ImGuiCol_PlotLinesHovered] = palette.AccentHover;
    colors[ImGuiCol_PlotHistogram] = palette.AccentAlt;
    colors[ImGuiCol_PlotHistogramHovered] = palette.AccentAltHover;

    colors[ImGuiCol_TableHeaderBg] = palette.Surface;
    colors[ImGuiCol_TableBorderStrong] = palette.Border;
    colors[ImGuiCol_TableBorderLight] = palette.Border;
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

    colors[ImGuiCol_TreeLines] = palette.Border;
    colors[ImGuiCol_DragDropTarget] = palette.Accent;
    colors[ImGuiCol_DragDropTargetBg] = palette.AccentHover;
    colors[ImGuiCol_UnsavedMarker] = palette.AccentAlt;

    colors[ImGuiCol_NavCursor] = palette.Accent;
    colors[ImGuiCol_NavWindowingHighlight] = palette.Accent;
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(palette.Background.x, palette.Background.y, palette.Background.z, 0.6f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(palette.Background.x, palette.Background.y, palette.Background.z, 0.6f);

    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
}

} // namespace Wankel
