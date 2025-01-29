#include "Application.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace Wankel {
	
	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		double time = 0;
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(10));
			time += 0.000001;
			std::cout << time << std::endl;
		}
	}
}