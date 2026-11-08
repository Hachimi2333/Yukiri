#pragma once

#include <Yukiri/Platform/Window.hpp>
#include <SDL3/SDL.h>

namespace Yukiri
{
struct Window::Impl
{
	SDL_Window* m_Native = nullptr;
	WindowConfig m_Config;
	bool m_Open = true;
	bool m_Dirty = true;
};
}
