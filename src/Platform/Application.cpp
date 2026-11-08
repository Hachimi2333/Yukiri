#include <Yukiri/Platform/Application.hpp>
#include <Yukiri/Platform/Window.hpp>
#include "WindowState.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <cassert>
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
	std::thread::id m_Thread = std::this_thread::get_id();
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
	return std::unique_ptr<Application>(new Application(std::move(impl)));
}

Application::~Application()
{
	m_Impl->m_Windows.clear();
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
	const auto flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY |
		(config.m_Visible ? 0 : SDL_WINDOW_HIDDEN) |
		(config.m_Decoration == WindowDecoration::Custom ? SDL_WINDOW_BORDERLESS : 0);
	impl->m_Native = SDL_CreateWindow(config.m_Title.c_str(), config.m_Width, config.m_Height, flags);
	if (!impl->m_Native)
		return std::unexpected(Error{ErrorCode::WindowCreation, SDL_GetError()});
	SDL_SetWindowMinimumSize(impl->m_Native, config.m_MinWidth, config.m_MinHeight);
	auto window = std::unique_ptr<Window>(new Window(std::move(impl)));
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
	bool dirty = false;
	for (const auto& window : m_Impl->m_Windows)
		dirty |= window->m_Impl->m_Dirty || !window->IsOpen();
	SDL_Event event;
	auto dispatch = [&](const SDL_Event& input)
	{
		if (input.type == SDL_EVENT_QUIT)
			Quit();
		for (const auto& window : m_Impl->m_Windows)
		{
			if (input.type >= SDL_EVENT_WINDOW_FIRST && input.type <= SDL_EVENT_WINDOW_LAST &&
				input.window.windowID == SDL_GetWindowID(window->m_Impl->m_Native))
			{
				if (input.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
					window->Close();
				else
					window->Invalidate();
			}
		}
	};
	if (wait && !dirty && !m_Impl->m_Windows.empty() && SDL_WaitEventTimeout(&event, 100))
		dispatch(event);
	while (SDL_PollEvent(&event))
		dispatch(event);
	std::erase_if(m_Impl->m_Windows, [](const auto& window) { return !window->IsOpen(); });
	for (const auto& window : m_Impl->m_Windows)
	{
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
}
