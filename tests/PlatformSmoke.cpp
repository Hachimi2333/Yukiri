#include <Yukiri/Platform/Application.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/Render/Canvas.hpp>

#include <iostream>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <SDL3/SDL.h>
#undef CreateWindow
#undef DrawText
#endif

class PaintProbe : public Yukiri::Widget
{
	void OnPaint(Yukiri::Canvas& canvas) override
	{
		canvas.FillRectangle({20, 20, 120, 80}, Yukiri::Color::FromHex(0x3674EA), 12);
		canvas.FillRectangle({70, 50, 120, 80}, Yukiri::Color::FromHex(0xE84C80, 0.5f), 8);
		canvas.StrokeRectangle({20, 150, 200, 60}, GetTheme().m_Accent, 2, 6);
		canvas.PushClip({200, 20, 50, 50});
		canvas.FillRectangle({180, 0, 100, 100}, Yukiri::Color::FromHex(0x35B78A), 20);
		canvas.PopClip();
		canvas.DrawLine({30, 240}, {260, 280}, GetTheme().m_Text, 2);
		canvas.DrawText("Yukiri / 中文字体回退", {20, 110}, GetTheme().m_Text, 16);
	}
};

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
	config.m_Decoration = Yukiri::WindowDecoration::Custom;
	auto second = application.CreateWindow(config);
	if (!first || !second)
	{
		std::cerr << (!first ? first.error().m_Message : second.error().m_Message) << '\n';
		return 4;
	}
	(*first)->SetContent(std::make_unique<PaintProbe>());
	(*second)->SetTheme(Yukiri::ThemeMode::Dark);
	(*second)->SetContent(std::make_unique<PaintProbe>());
#if defined(_WIN32)
	int windowCount = 0;
	auto** nativeWindows = SDL_GetWindows(&windowCount);
	bool maxHitVerified = false;
	for (int i = 0; i < windowCount; ++i)
	{
		if (!(SDL_GetWindowFlags(nativeWindows[i]) & SDL_WINDOW_BORDERLESS)) continue;
		auto handle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(nativeWindows[i]), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
		RECT client{};
		GetClientRect(handle, &client);
		const float scale = static_cast<float>(GetDpiForWindow(handle)) / 96;
		POINT point{client.right - static_cast<LONG>(69 * scale), static_cast<LONG>(20 * scale)};
		ClientToScreen(handle, &point);
		maxHitVerified = SendMessageW(handle, WM_NCHITTEST, 0, MAKELPARAM(point.x, point.y)) == HTMAXBUTTON;
	}
	SDL_free(nativeWindows);
	if (!maxHitVerified) return 9;
#endif
	for (int frame = 0; frame < 8; ++frame)
	{
		if (frame == 3) { auto resized = (*second)->SetSize({640, 420}); if (!resized) return 10; }
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
	if (application.GetValidationErrorCount() != 0)
		return 8;
	std::cout << "Application lifecycle and two-window smoke passed.\n";
}
