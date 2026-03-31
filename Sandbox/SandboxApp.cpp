#include <Wankel.h>
#include <iostream>


class ExampleLayer : public Wankel::Layer {
public:
	ExampleLayer() : Layer("Example") {
	}

	void OnUpdate() override {
		WK_CORE_INFO("ExampleLayer::Update");
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
