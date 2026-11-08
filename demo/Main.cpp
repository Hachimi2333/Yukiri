#include "Gallery.hpp"
#include <iostream>
#include <string_view>
#include <thread>
#include <chrono>

int main(int argc, char** argv)
{
	bool smoke = false;
	auto decoration = Yukiri::WindowDecoration::Custom;
	bool idle = false;
	for (int i = 1; i < argc; ++i)
	{
		if (std::string_view(argv[i]) == "--smoke") smoke = true;
		if (std::string_view(argv[i]) == "--custom") decoration = Yukiri::WindowDecoration::Custom;
		if (std::string_view(argv[i]) == "--native") decoration = Yukiri::WindowDecoration::Native;
		if (std::string_view(argv[i]) == "--idle-smoke") idle = true;
	}
	auto application = Yukiri::Application::Create({"Yukiri Demo", true});
	if (!application) { std::cerr << application.error().m_Message << '\n'; return 1; }
	auto& app = **application;
	auto mainWindow = YukiriDemo::OpenGallery(app, decoration,
		Yukiri::ThemeMode::System, !smoke && !idle);
	if (!mainWindow) { std::cerr << mainWindow.error().m_Message << '\n'; return 2; }
	if (idle)
	{
		for (int i = 0; i < 12; ++i)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(450));
			auto polled = app.Poll(false);
			if (!polled) return 10;
		}
		app.Quit();
		auto closed = app.Poll(false);
		return closed && !*closed && !app.GetValidationErrorCount() ? 0 : 11;
	}
	if (smoke)
	{
		const auto otherDecoration = decoration == Yukiri::WindowDecoration::Custom ?
			Yukiri::WindowDecoration::Native : Yukiri::WindowDecoration::Custom;
		auto second = YukiriDemo::OpenGallery(app, otherDecoration, Yukiri::ThemeMode::Dark, false);
		if (!second) { std::cerr << second.error().m_Message << '\n'; return 3; }
		for (int frame = 0; frame < 70; ++frame)
		{
			auto* gallery = static_cast<YukiriDemo::Gallery*>((*mainWindow)->GetContent());
			gallery->Exercise(frame);
			if (!gallery->IsThemeSynchronized() || (*second)->GetConfig().m_Theme != Yukiri::ThemeMode::Dark) return 12;
			if (frame == 15) { auto size = (*mainWindow)->SetSize({360, 520}); if (!size) return 4; }
			if (frame == 30) { auto size = (*mainWindow)->SetSize({1040, 740}); if (!size) return 5; }
			(*mainWindow)->Invalidate();
			(*second)->Invalidate();
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
			do
			{
				(*mainWindow)->Invalidate();
				(*second)->Invalidate();
				auto polled = app.Poll(true);
				if (!polled) { std::cerr << polled.error().m_Message << '\n'; return 6; }
			} while (std::chrono::steady_clock::now() < deadline);
		}
		app.Quit();
		auto closed = app.Poll(false);
		if (!closed || *closed || app.GetValidationErrorCount()) return 7;
		std::cout << "Gallery GPU smoke passed: native/custom, resize, themes, text, tabs, and controls.\n";
		return 0;
	}
	auto run = app.Run();
	if (!run) { std::cerr << run.error().m_Message << '\n'; return 8; }
	return app.GetValidationErrorCount() ? 9 : 0;
}
