#include <Yukiri/Yukiri.hpp>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;
struct Samples
{
	Clock::time_point m_Start = Clock::now();
	std::vector<double> m_Intervals;
	Clock::time_point m_Last{};
	unsigned m_Frames = 0;
};
class GalleryProbe final : public Yukiri::StackPanel
{
public:
	explicit GalleryProbe(Samples& samples) : StackPanel(Yukiri::Orientation::Vertical, 6), m_Samples(samples) {}
protected:
	void OnPaint(Yukiri::Canvas& canvas) override
	{
		const auto now = Clock::now();
		if (now - m_Samples.m_Start > std::chrono::seconds(1) && m_Samples.m_Last != Clock::time_point{})
			m_Samples.m_Intervals.push_back(std::chrono::duration<double, std::milli>(now - m_Samples.m_Last).count());
		m_Samples.m_Last = now;
		++m_Samples.m_Frames;
		StackPanel::OnPaint(canvas);
	}
private:
	Samples& m_Samples;
};
struct Stimulus { SDL_WindowID m_Window; bool m_Alternate = false; float m_Scale; };
Uint32 SDLCALL Hover(void* state, SDL_TimerID, Uint32 interval)
{
	auto& stimulus = *static_cast<Stimulus*>(state);
	stimulus.m_Alternate = !stimulus.m_Alternate;
	SDL_Event event{};
	event.type = SDL_EVENT_MOUSE_MOTION;
	event.motion.windowID = stimulus.m_Window;
	event.motion.x = 40 * stimulus.m_Scale;
	event.motion.y = (stimulus.m_Alternate ? 30.0f : 70.0f) * stimulus.m_Scale;
	SDL_PushEvent(&event);
	return interval;
}
}

int main(int argc, char** argv)
{
	const int seconds = argc > 1 ? std::clamp(std::atoi(argv[1]), 2, 60) : 8;
	auto created = Yukiri::Application::Create({"Yukiri frame benchmark", true});
	if (!created) { std::cerr << created.error().m_Message << '\n'; return 1; }
	auto& app = **created;
	Yukiri::WindowConfig config;
	config.m_Title = "Yukiri - Frame Benchmark";
	config.m_Width = 800;
	config.m_Height = 700;
	auto opened = app.CreateWindow(config);
	if (!opened) { std::cerr << opened.error().m_Message << '\n'; return 2; }
	Samples samples;
	auto root = std::make_unique<GalleryProbe>(samples);
	root->SetPadding(Yukiri::Insets(16));
	root->AddChild<Yukiri::Button>("Animated hover A / 动画 A");
	root->AddChild<Yukiri::Button>("Animated hover B / 动画 B");
	for (int row = 0; row < 12; ++row)
		root->AddChild<Yukiri::Label>("Stable text shaping / 稳定文本塑形 " + std::to_string(row));
	(*opened)->SetContent(std::move(root));
	int count = 0;
	auto** windows = SDL_GetWindows(&count);
	if (!windows || count != 1) { SDL_free(windows); return 3; }
	Stimulus stimulus{SDL_GetWindowID(windows[0]), false, (*opened)->GetScale()};
	SDL_free(windows);
	const auto timer = SDL_AddTimer(80, Hover, &stimulus);
	if (!timer) return 4;
	bool failed = false;
	while (Clock::now() - samples.m_Start < std::chrono::seconds(seconds))
	{
		auto polled = app.Poll(true);
		if (!polled || !*polled) { failed = true; break; }
	}
	SDL_RemoveTimer(timer);
	app.Quit();
	auto closed = app.Poll(false);
	if (!closed || failed || app.GetValidationErrorCount()) return 5;
	std::ranges::sort(samples.m_Intervals);
	const auto total = std::chrono::duration<double>(Clock::now() - samples.m_Start).count();
	if (samples.m_Intervals.empty()) return 6;
	auto percentile = [&](double fraction) { return samples.m_Intervals[static_cast<std::size_t>(fraction * static_cast<double>(samples.m_Intervals.size() - 1))]; };
	std::cout << "Benchmark frames=" << samples.m_Frames << " elapsed_s=" << total << " warm_interval_p50_ms=" << percentile(0.5)
		<< " warm_interval_p95_ms=" << percentile(0.95) << " validation_errors=" << app.GetValidationErrorCount() << '\n';
}
