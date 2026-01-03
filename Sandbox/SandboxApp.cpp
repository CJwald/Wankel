#include <Wankel.h>
#include <iostream>


class Sandbox : public Wankel::Application {
public:
	Sandbox() {};
	~Sandbox() {};


};


Wankel::Application* Wankel::CreateApplication() {
	return new Sandbox();
}
