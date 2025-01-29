#pragma once

#ifdef WK_PLATFORM_WINDOWS

extern Wankel::Application* Wankel::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Wankel::CreateApplication();
	app->Run();
	delete app;
}

#endif