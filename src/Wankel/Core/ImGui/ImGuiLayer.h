#pragma once

#include "Wankel/Core/Layer.h"

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
	};

}
