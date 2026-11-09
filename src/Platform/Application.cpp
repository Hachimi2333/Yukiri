#include <Yukiri/Platform/Application.hpp>
#include <Yukiri/Platform/Window.hpp>
#include "WindowState.hpp"
#include "Core/DispatchQueue.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <thread>
#include <vector>

namespace Yukiri
{
namespace
{
bool ApplicationActive = false;
}

struct Application::Impl
{
	ApplicationConfig m_Config;
	std::vector<std::unique_ptr<Window>> m_Windows;
	std::unique_ptr<Detail::RenderDevice> m_Device;
	std::unique_ptr<Detail::TextEngine> m_Text;
	std::thread::id m_Thread = std::this_thread::get_id();
	bool m_Polling = false;
	std::unique_ptr<Detail::DispatchQueue> m_Dispatch;
	std::vector<std::shared_ptr<Command>> m_Commands;
};

Application::Application(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl)) {}

Result<std::unique_ptr<Application>> Application::Create(const ApplicationConfig& config)
{
	if (ApplicationActive)
		return std::unexpected(Error{ErrorCode::PlatformInitialization, "Only one Application may be active."});
	SDL_SetMainReady();
	if (!SDL_SetHintWithPriority(SDL_HINT_IME_IMPLEMENTED_UI, "composition", SDL_HINT_OVERRIDE))
		return std::unexpected(Error{ErrorCode::PlatformInitialization, "Could not enable application-rendered IME composition."});
	SDL_SetAppMetadata(config.m_Name.c_str(), "0.1.0", "org.yukiri.application");
	if (!SDL_Init(SDL_INIT_VIDEO))
		return std::unexpected(Error{ErrorCode::PlatformInitialization, SDL_GetError()});
	ApplicationActive = true;
	auto impl = std::make_unique<Impl>();
	impl->m_Config = config;
	const auto wakeEvent = SDL_RegisterEvents(1);
	if (!wakeEvent)
	{
		SDL_Quit();
		ApplicationActive = false;
		return std::unexpected(Error{ErrorCode::PlatformInitialization, "Registering the dispatcher event failed."});
	}
	impl->m_Dispatch = std::make_unique<Detail::DispatchQueue>([wakeEvent]
	{
		SDL_Event event{};
		event.type = wakeEvent;
		SDL_PushEvent(&event);
	});
	auto text = Detail::TextEngine::Create();
	if (!text)
	{
		SDL_Quit();
		ApplicationActive = false;
		return std::unexpected(text.error());
	}
	impl->m_Text = std::move(*text);
	return std::unique_ptr<Application>(new Application(std::move(impl)));
}

Application::~Application()
{
	m_Impl->m_Dispatch->Stop();
	m_Impl->m_Windows.clear();
	m_Impl->m_Text.reset();
	m_Impl->m_Device.reset();
	SDL_Quit();
	ApplicationActive = false;
}

Result<Window*> Application::CreateWindow(const WindowConfig& config)
{
	assert(m_Impl->m_Thread == std::this_thread::get_id());
	if (config.m_Width <= 0 || config.m_Height <= 0 || config.m_MinWidth <= 0 || config.m_MinHeight <= 0)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "Window dimensions must be positive."});
	auto impl = std::make_unique<Window::Impl>();
	impl->m_Config = config;
	impl->m_Text = m_Impl->m_Text.get();
	impl->m_Motion = m_Impl->m_Config.m_Motion;
	impl->m_Tree.m_ApplicationCommand = [this](KeyEvent& event)
	{
		const auto commands = m_Impl->m_Commands;
		for (const auto& command : commands) if (command->TryExecute(event)) return true;
		return false;
	};
	const auto flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY |
		(config.m_Visible ? 0 : SDL_WINDOW_HIDDEN) |
		(config.m_Decoration == WindowDecoration::Custom ? SDL_WINDOW_BORDERLESS : 0);
	const float scale = std::max(1.0f, SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay()));
	impl->m_Native = SDL_CreateWindow(config.m_Title.c_str(), static_cast<int>(std::lround(static_cast<float>(config.m_Width) * scale)),
		static_cast<int>(std::lround(static_cast<float>(config.m_Height) * scale)), flags);
	if (!impl->m_Native)
		return std::unexpected(Error{ErrorCode::WindowCreation, SDL_GetError()});
	SDL_SetWindowMinimumSize(impl->m_Native, static_cast<int>(std::lround(static_cast<float>(config.m_MinWidth) * scale)),
		static_cast<int>(std::lround(static_cast<float>(config.m_MinHeight) * scale)));
	auto window = std::unique_ptr<Window>(new Window(std::move(impl)));
	auto events = Detail::WindowEvents::Create(window->m_Impl->m_Native,
		[observer = window.get()] { observer->m_Impl->m_Tree.CancelPointer(); });
	if (!events) return std::unexpected(events.error());
	window->m_Impl->m_Events = std::move(*events);
	if (!m_Impl->m_Device)
	{
		auto device = Detail::RenderDevice::Create(window->m_Impl->m_Native, m_Impl->m_Config.m_EnableValidation);
		if (!device) return std::unexpected(device.error());
		m_Impl->m_Device = std::move(*device);
		m_Impl->m_Text->SetDevice(m_Impl->m_Device.get());
	}
	if (config.m_Decoration == WindowDecoration::Custom)
	{
		auto chrome = Detail::WindowChrome::Create(*window, window->m_Impl->m_Native, *m_Impl->m_Device);
		if (!chrome) return std::unexpected(chrome.error());
		window->m_Impl->m_Chrome = std::move(*chrome);
	}
	auto renderer = Detail::WindowRenderer::Create(*m_Impl->m_Device, window->m_Impl->m_Native);
	if (!renderer) return std::unexpected(renderer.error());
	window->m_Impl->m_Renderer = std::move(*renderer);
	auto* observer = window.get();
	m_Impl->m_Windows.push_back(std::move(window));
	return observer;
}

Result<void> Application::Run()
{
	for (;;)
	{
		auto result = Poll(true);
		if (!result)
			return std::unexpected(result.error());
		if (!*result)
			return {};
	}
}

Result<bool> Application::Poll(bool wait)
{
	assert(m_Impl->m_Thread == std::this_thread::get_id());
	if (m_Impl->m_Polling)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "Application::Poll cannot be called reentrantly."});
	m_Impl->m_Polling = true;
	struct PollScope
	{
		bool& m_Polling;
		~PollScope() { m_Polling = false; }
	} scope{m_Impl->m_Polling};
	SDL_Event event;
	auto dispatch = [&](const SDL_Event& input)
	{
		if (input.type == SDL_EVENT_QUIT)
			Quit();
		if (input.type == SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED)
			for (const auto& window : m_Impl->m_Windows) window->m_Impl->RefreshRate();
		if (input.type == SDL_EVENT_SYSTEM_THEME_CHANGED)
			for (const auto& window : m_Impl->m_Windows)
				if (window->GetConfig().m_Theme == ThemeMode::System) window->SetTheme(ThemeMode::System);
		Window* target = nullptr;
		auto* native = SDL_GetWindowFromEvent(&input);
		for (const auto& window : m_Impl->m_Windows)
		{
			if (window->m_Impl->m_Native == native) { target = window.get(); break; }
		}
		if (target && target->IsOpen()) target->m_Impl->HandleEvent(input);
	};
	if (wait && !m_Impl->m_Windows.empty())
	{
		const auto now = Detail::FramePacer::Clock::now();
		auto deadline = m_Impl->m_Dispatch->NextDeadline();
		for (const auto& window : m_Impl->m_Windows) deadline = std::min(deadline, window->m_Impl->NextUpdate(now));
		if (deadline == Detail::FramePacer::Time::max())
		{
			if (SDL_WaitEvent(&event)) dispatch(event);
		}
		else if (deadline > now)
		{
			const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
			if (milliseconds > 0)
			{
				if (SDL_WaitEventTimeout(&event, static_cast<Sint32>(std::min<std::int64_t>(milliseconds, 1000)))) dispatch(event);
			}
			else if (!SDL_PollEvent(&event)) SDL_DelayPrecise(static_cast<Uint64>(std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now).count()));
			else dispatch(event);
		}
	}
	// Bound event processing so a continuous input stream cannot starve animation frames.
	for (int count = 0; count < 256 && SDL_PollEvent(&event); ++count) dispatch(event);
	m_Impl->m_Dispatch->RunReady();
	std::vector<Window*> requested;
	for (const auto& window : m_Impl->m_Windows)
		if (window->m_Impl->m_Close.m_Pending) requested.push_back(window.get());
	for (auto* window : requested)
	{
		window->m_Impl->m_Close.Process(window->m_CloseRequested);
	}
	std::vector<Window*> closing;
	for (const auto& window : m_Impl->m_Windows)
	{
		if (!window->IsOpen() && !window->m_Impl->m_Close.m_Notified)
		{
			window->m_Impl->m_Close.m_Notified = true;
			closing.push_back(window.get());
		}
	}
	for (auto* window : closing) window->m_Closed.Emit();
	for (const auto& window : m_Impl->m_Windows)
		if (window->m_Impl->m_ParentWindow && !window->m_Impl->m_ParentWindow->IsOpen())
		{
			auto cleared = window->SetOwner(nullptr);
			if (!cleared) return std::unexpected(cleared.error());
		}
	std::erase_if(m_Impl->m_Windows, [](const auto& window) { return !window->IsOpen() && window->m_Impl->m_Close.m_Notified; });
	if (!closing.empty() && m_Impl->m_Windows.empty()) m_Impl->m_Dispatch->Stop();
	std::vector<Window*> windows;
	for (const auto& window : m_Impl->m_Windows) windows.push_back(window.get());
	for (auto* window : windows)
	{
		if (!window->IsOpen()) continue;
		auto result = window->Update(wait);
		if (!result)
			return std::unexpected(result.error());
	}
	return !m_Impl->m_Windows.empty();
}

Dispatcher Application::GetDispatcher() const { return m_Impl->m_Dispatch->GetDispatcher(); }
void Application::AddCommand(std::shared_ptr<Command> command) { if (command) m_Impl->m_Commands.push_back(std::move(command)); }
void Application::RemoveCommand(const Command& command) { std::erase_if(m_Impl->m_Commands, [&](const auto& entry) { return entry.get() == &command; }); }

void Application::Quit()
{
	for (const auto& window : m_Impl->m_Windows)
		window->Close();
}

std::uint32_t Application::GetValidationErrorCount() const noexcept
{
	return m_Impl->m_Device ? m_Impl->m_Device->m_ValidationErrors : 0;
}

Result<void> Application::LoadFont(const std::filesystem::path& path, long faceIndex)
{
	auto loaded = m_Impl->m_Text->LoadFont(path, faceIndex);
	if (!loaded) return loaded;
	for (const auto& window : m_Impl->m_Windows)
	{
		window->m_Impl->m_Tree.m_LayoutDirty = true;
		window->Invalidate();
	}
	return {};
}
}
