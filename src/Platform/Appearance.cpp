#include "Appearance.hpp"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#endif

namespace Yukiri::Detail
{
bool IsSystemDark() { return SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK; }
void ApplyAppearance(SDL_Window* window, bool dark)
{
#if defined(_WIN32)
	auto handle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
	if (handle)
	{
		const BOOL enabled = dark ? TRUE : FALSE;
		DwmSetWindowAttribute(handle, 20, &enabled, sizeof(enabled));
	}
#else
	(void)window;
	(void)dark;
#endif
}
}
