#include <Yukiri/Platform/Application.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/Render/Canvas.hpp>

#include <iostream>

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
	auto second = application.CreateWindow(config);
	if (!first || !second)
	{
		std::cerr << (!first ? first.error().m_Message : second.error().m_Message) << '\n';
		return 4;
	}
	(*first)->SetContent(std::make_unique<PaintProbe>());
	(*second)->SetTheme(Yukiri::ThemeMode::Dark);
	(*second)->SetContent(std::make_unique<PaintProbe>());
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
	if (application.GetValidationErrorCount() != 0)
		return 8;
	std::cout << "Application lifecycle and two-window smoke passed.\n";
}
