#include "DebugOverlay.h"
#include <imgui.h>

namespace Wankel {

float DebugOverlay::s_FrameTimes[s_HistorySize] = {};
int DebugOverlay::s_FrameIndex = 0;

void DebugOverlay::PushFrameTime(float dt) {
    s_FrameTimes[s_FrameIndex] = dt;
    s_FrameIndex = (s_FrameIndex + 1) % s_HistorySize;
}

void DebugOverlay::DrawFPSPanel() {
    ImGui::Begin("Performance");

    float dt = s_FrameTimes[(s_FrameIndex + s_HistorySize - 1) % s_HistorySize];
    float fps = dt > 0.0f ? 1.0f / dt : 0.0f;

    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.3f ms", dt * 1000.0f);

    float values[s_HistorySize];

    for (int i = 0; i < s_HistorySize; i++) {
        int index = (s_FrameIndex + i) % s_HistorySize;
        values[i] = s_FrameTimes[index] * 1000.0f;
    }

    ImGui::PlotLines(
        "Frame Time (ms)",
        values,
        s_HistorySize,
        0,
        nullptr,
        0.0f,
        33.0f,
        ImVec2(0, 80)
    );

    ImGui::End();
}

}
