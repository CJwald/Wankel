#pragma once


extern Wankel::Application* Wankel::CreateApplication();

int main(int argc, char** argv) {
	
	Wankel::Log::Init(); // this wont live here forever, just a test
	WK_CORE_WARNING("Initialized Logger");
	WK_CLIENT_WARNING("Initialized Logger");
	WK_SERVER_WARNING("Initialized Logger");

	auto app = Wankel::CreateApplication();
	app->Run();
	delete app;
}


