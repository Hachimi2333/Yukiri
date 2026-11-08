#include <Yukiri/Platform/Application.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <Yukiri/Controls/TextBox.hpp>
#include <Yukiri/Controls/ScrollView.hpp>
#include <Yukiri/Layout/StackPanel.hpp>
#include <chrono>
#include <cmath>

#include <iostream>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <SDL3/SDL.h>
#undef CreateWindow
#undef DrawText
#endif

class PaintProbe : public Yukiri::Widget
{
protected:
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

struct ReplacementState
{
	bool m_Destroyed = false;
	bool m_DestroyedEarly = false;
};
class ReplacingPaintProbe : public PaintProbe
{
public:
	explicit ReplacingPaintProbe(std::shared_ptr<ReplacementState> state) : m_State(std::move(state)) {}
	~ReplacingPaintProbe() override { m_State->m_Destroyed = true; }
protected:
	void OnPaint(Yukiri::Canvas& canvas) override
	{
		PaintProbe::OnPaint(canvas);
		GetWindow()->SetContent(std::make_unique<PaintProbe>());
		m_State->m_DestroyedEarly = m_State->m_Destroyed;
	}
private:
	std::shared_ptr<ReplacementState> m_State;
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
#if defined(_WIN32)
	if (std::string_view(SDL_GetHint(SDL_HINT_IME_IMPLEMENTED_UI)) != "composition") return 15;
#endif
	if (Yukiri::Application::Create())
		return 2;
	Yukiri::WindowConfig invalid;
	invalid.m_Width = 0;
	if (application.CreateWindow(invalid))
		return 3;
	Yukiri::WindowConfig config;
	config.m_Title = "Yukiri smoke custom";
	config.m_Visible = false;
	config.m_Width = 480;
	config.m_Height = 320;
	auto first = application.CreateWindow(config);
	config.m_Decoration = Yukiri::WindowDecoration::Native;
	config.m_Title = "Yukiri smoke native";
	auto second = application.CreateWindow(config);
	if (!first || !second)
	{
		std::cerr << (!first ? first.error().m_Message : second.error().m_Message) << '\n';
		return 4;
	}
	auto replacement = std::make_shared<ReplacementState>();
	(*first)->SetContent(std::make_unique<ReplacingPaintProbe>(replacement));
	(*second)->SetTheme(Yukiri::ThemeMode::Dark);
	(*second)->SetContent(std::make_unique<PaintProbe>());
	std::cout << "Window UI scale: " << (*first)->GetScale() << '\n';
	bool firstClosed = false;
	bool secondClosed = false;
	bool reentryRejected = false;
	auto firstConnection = (*first)->m_Closed.Connect([&]
	{
		firstClosed = true;
		auto nested = application.Poll(false);
		reentryRejected = !nested && nested.error().m_Code == Yukiri::ErrorCode::InvalidArgument;
	});
	auto secondConnection = (*second)->m_Closed.Connect([&] { secondClosed = true; });
#if defined(_WIN32)
	int windowCount = 0;
	auto** nativeWindows = SDL_GetWindows(&windowCount);
	bool maxHitVerified = false;
	bool overlappedVerified = false;
	for (int i = 0; i < windowCount; ++i)
	{
		if (!(SDL_GetWindowFlags(nativeWindows[i]) & SDL_WINDOW_BORDERLESS)) continue;
		auto handle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(nativeWindows[i]), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
		if (std::string_view(SDL_GetWindowTitle(nativeWindows[i])) != "Yukiri smoke custom") return 21;
		RECT client{};
		GetClientRect(handle, &client);
		const float scale = static_cast<float>(GetDpiForWindow(handle)) / 96;
		POINT point{client.right - static_cast<LONG>(69 * scale), static_cast<LONG>(20 * scale)};
		ClientToScreen(handle, &point);
		maxHitVerified = SendMessageW(handle, WM_NCHITTEST, 0, MAKELPARAM(point.x, point.y)) == HTMAXBUTTON;
		overlappedVerified = (GetWindowLongPtrW(handle, GWL_STYLE) & WS_POPUP) == 0;
	}
	SDL_free(nativeWindows);
	if (!maxHitVerified || !overlappedVerified) return 9;
	if ((*first)->GetConfig().m_Decoration != Yukiri::WindowDecoration::Custom || (*first)->GetContentBounds().m_Y <= 0 ||
		(*second)->GetContentBounds().m_Y != 0) return 22;
#endif
	auto primed = application.Poll(false);
	auto repainted = application.Poll(true);
	if (!primed || !repainted || !replacement->m_Destroyed || replacement->m_DestroyedEarly ||
		(*first)->GetContent()->GetBounds().Empty()) return 11;
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
#if defined(_WIN32)
	auto scroll = std::make_unique<Yukiri::ScrollView>();
	auto* scrolling = scroll.get();
	auto contents = std::make_unique<Yukiri::StackPanel>();
	auto& editor = contents->AddChild<Yukiri::TextBox>("IME caret probe");
	contents->AddChild<Yukiri::Widget>().SetHeight(1000);
	scrolling->SetContent(std::move(contents));
	(*first)->SetContent(std::move(scroll));
	editor.RequestFocus();
	auto** imeWindows = SDL_GetWindows(&windowCount);
	SDL_Window* imeWindow = nullptr;
	for (int i = 0; i < windowCount; ++i)
		if (std::string_view(SDL_GetWindowTitle(imeWindows[i])) == "Yukiri smoke custom") imeWindow = imeWindows[i];
	SDL_free(imeWindows);
	if (!imeWindow) return 12;
	auto pumpUntil = [&](auto predicate)
	{
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		do
		{
			auto polled = application.Poll(false);
			if (!polled) return false;
			if (predicate()) return true;
			SDL_Delay(1);
		} while (std::chrono::steady_clock::now() < deadline);
		return false;
	};
	SDL_Rect before{};
	int cursor = 0;
	if (!pumpUntil([&] { return SDL_GetTextInputArea(imeWindow, &before, &cursor) && before.h > 1; })) return 13;
	const auto originalText = editor.GetText();
	SDL_Event composition{};
	composition.type = SDL_EVENT_TEXT_EDITING;
	composition.edit.windowID = SDL_GetWindowID(imeWindow);
	composition.edit.text = "ni'hao";
	composition.edit.start = 3;
	composition.edit.length = 3;
	if (!SDL_PushEvent(&composition)) return 16;
	const int expectedCaret = before.x + static_cast<int>(std::lround((*first)->GetTextCaretX(
		originalText + "ni'hao", originalText.size() + 3, editor.GetFontSize()) * (*first)->GetScale())) -
		static_cast<int>(std::lround((*first)->GetTextCaretX(originalText, originalText.size(), editor.GetFontSize()) * (*first)->GetScale()));
	if (!pumpUntil([&]
	{
		SDL_Rect area{};
		return SDL_GetTextInputArea(imeWindow, &area, &cursor) && std::abs(area.x - expectedCaret) <= 2;
	}) || editor.GetText() != originalText) return 17;
	std::cout << "IME preedit event updates the displayed caret without committing text.\n";
	editor.SetText(std::string(160, 'x'));
	composition.edit.start = 6;
	composition.edit.length = 0;
	if (!SDL_PushEvent(&composition)) return 18;
	if (!pumpUntil([&]
	{
		SDL_Rect area{};
		const float right = editor.GetBounds().Right() * (*first)->GetScale();
		return SDL_GetTextInputArea(imeWindow, &area, &cursor) && area.x > right - 30 * (*first)->GetScale() && area.x < right;
	})) return 19;
	editor.SetText(originalText);
	if (!pumpUntil([&]
	{
		SDL_Rect area{};
		return SDL_GetTextInputArea(imeWindow, &area, &cursor) && std::abs(area.x - before.x) <= 2;
	})) return 20;
	scrolling->SetOffset({0, 200});
	const int expectedY = before.y - static_cast<int>(std::lround(200 * (*first)->GetScale()));
	if (!pumpUntil([&]
	{
		SDL_Rect after{};
		return SDL_GetTextInputArea(imeWindow, &after, &cursor) && std::abs(after.y - expectedY) <= 1;
	})) return 14;
	std::cout << "Clipped text input caret follows scroll translation.\n";
#endif
	(*first)->Close();
	auto remaining = application.Poll(false);
	if (!remaining || !*remaining || !firstClosed || !reentryRejected || secondClosed)
		return 6;
	application.Quit();
	auto finished = application.Poll(false);
	if (!finished || *finished || !secondClosed)
		return 7;
	if (application.GetValidationErrorCount() != 0)
		return 8;
	std::cout << "Application lifecycle and two-window smoke passed.\n";
}
