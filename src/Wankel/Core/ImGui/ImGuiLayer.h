#pragma once

#include "Wankel/Core/Layer.h"
#include "Wankel/Renderer/GPUTimerQuery.h"

struct GLFWwindow;

namespace Wankel {

class ImGuiLayer : public Layer {
public:
    ImGuiLayer();
    ~ImGuiLayer() = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnEvent(Event& e) override;

    void Begin();
    void End();

    // GPU cost of ImGui's own render (End()'s ImGui_ImplOpenGL3_RenderDrawData call), polled exactly
    // once per frame inside End() itself - see GPUTimerQuery's own comment on why PollElapsedMs()
    // must only ever be called once per frame (it swaps the double-buffer slot on every call).
    float GetGpuMs() const { return m_LastGpuMs; }

private:
    GPUTimerQuery m_GpuTimer;
    float m_LastGpuMs = 0.0f;
};

} // namespace Wankel
