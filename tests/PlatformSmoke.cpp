#include <Yukiri/Platform/Application.hpp>
#include <Yukiri/Platform/Window.hpp>

#include <iostream>

int main()
{
	auto result = Yukiri::Application::Create();
	if (!result)
	{
		std::cerr << result.error().m_Message << '\n';
		return 1;
	}
	auto& application = **result;
	if (Yukiri::Application::Create())
		return 2;
	Yukiri::WindowConfig invalid;
	invalid.m_Width = 0;
	if (application.CreateWindow(invalid))
		return 3;
	Yukiri::WindowConfig config;
	config.m_Visible = false;
	config.m_Width = 480;
	config.m_Height = 320;
	auto first = application.CreateWindow(config);
	auto second = application.CreateWindow(config);
	if (!first || !second)
	{
		std::cerr << (!first ? first.error().m_Message : second.error().m_Message) << '\n';
		return 4;
	}
	for (int frame = 0; frame < 8; ++frame)
	{
		(*first)->Invalidate();
		(*second)->Invalidate();
		auto polled = application.Poll(false);
		if (!polled || !*polled)
		{
			if (!polled) std::cerr << polled.error().m_Message << '\n';
			return 5;
		}
	}
	(*first)->Close();
	auto remaining = application.Poll(false);
	if (!remaining || !*remaining)
		return 6;
	application.Quit();
	auto finished = application.Poll(false);
	if (!finished || *finished)
		return 7;
	std::cout << "Application lifecycle and two-window smoke passed.\n";
}
