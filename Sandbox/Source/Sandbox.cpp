#include "Wankel/Wankel.h"


class Sandbox : public Wankel::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{

	}
};

Wankel::Application* Wankel::CreateApplication()
{
	return new Sandbox();
}