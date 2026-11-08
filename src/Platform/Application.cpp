#include <Yukiri/Platform/Application.hpp>
#include <Yukiri/Platform/Window.hpp>
#include "WindowState.hpp"

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
};

Application::Application(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl)) {}

Result<std::unique_ptr<Application>> Application::Create(const ApplicationConfig& config)
{
	if (ApplicationActive)
		return std::unexpected(Error{ErrorCode::PlatformInitialization, "Only one Application may be active."});
	SDL_SetMainReady();
	SDL_SetAppMetadata(config.m_Name.c_str(), "0.1.0", "org.yukiri.application");
	if (!SDL_Init(SDL_INIT_VIDEO))
		return std::unexpected(Error{ErrorCode::PlatformInitialization, SDL_GetError()});
	ApplicationActive = true;
	auto impl = std::make_unique<Impl>();
	impl->m_Config = config;
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
	if (config.m_Decoration == WindowDecoration::Custom)
	{
		auto chrome = Detail::WindowChrome::Create(*window, window->m_Impl->m_Native);
		if (!chrome) return std::unexpected(chrome.error());
		window->m_Impl->m_Chrome = std::move(*chrome);
	}
	if (!m_Impl->m_Device)
	{
		auto device = Detail::RenderDevice::Create(window->m_Impl->m_Native, m_Impl->m_Config.m_EnableValidation);
		if (!device) return std::unexpected(device.error());
		m_Impl->m_Device = std::move(*device);
		m_Impl->m_Text->SetDevice(m_Impl->m_Device.get());
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
	bool dirty = false;
	bool animating = false;
	for (const auto& window : m_Impl->m_Windows)
	{
		dirty |= window->m_Impl->m_Dirty || window->m_Impl->m_Tree.m_Dirty || !window->IsOpen();
		animating |= window->m_Impl->m_Tree.m_Animating;
	}
	SDL_Event event;
	auto dispatch = [&](const SDL_Event& input)
	{
		if (input.type == SDL_EVENT_QUIT)
			Quit();
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
	if (wait && !dirty && !m_Impl->m_Windows.empty() && SDL_WaitEventTimeout(&event, animating ? 16 : 500))
		dispatch(event);
	while (SDL_PollEvent(&event))
		dispatch(event);
	std::vector<Window*> closing;
	for (const auto& window : m_Impl->m_Windows)
	{
		if (!window->IsOpen() && !window->m_Impl->m_CloseNotified)
		{
			window->m_Impl->m_CloseNotified = true;
			closing.push_back(window.get());
		}
	}
	for (auto* window : closing) window->m_Closed.Emit();
	std::erase_if(m_Impl->m_Windows, [](const auto& window) { return !window->IsOpen() && window->m_Impl->m_CloseNotified; });
	std::vector<Window*> windows;
	for (const auto& window : m_Impl->m_Windows) windows.push_back(window.get());
	for (auto* window : windows)
	{
		if (!window->IsOpen()) continue;
		auto result = window->Update();
		if (!result)
			return std::unexpected(result.error());
	}
	return !m_Impl->m_Windows.empty();
}

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
