#include <Wankel.h>
#include <imgui.h>
#include <iostream>


class ExampleLayer : public Wankel::Layer {
public:
	ExampleLayer() : Layer("Example") {
	}

	void OnUpdate() override {
	}

	void OnImGuiRender() override {
        // Simple ImGui window
        ImGui::Begin("Test Window");

        ImGui::Text("Hello from ImGui!");
        ImGui::Text("This means ImGui is working 🎉");

        static float value = 0.0f;
        ImGui::SliderFloat("Float", &value, 0.0f, 1.0f);

        if (ImGui::Button("Click Me")) {
            printf("Button clicked!\n");
        }

        ImGui::End();
    }

	void OnEvent(Wankel::Event& event) override {
		WK_CORE_TRACE("{0}", event.ToString());
	}
};

class Sandbox : public Wankel::Application {
public:
	Sandbox() {
		PushLayer(new ExampleLayer());
	};
	~Sandbox() {};
};


Wankel::Application* Wankel::CreateApplication() {
	return new Sandbox();
}
