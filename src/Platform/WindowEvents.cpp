#include "WindowEvents.hpp"
#include <SDL3/SDL.h>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#endif

namespace Yukiri::Detail
{
struct WindowEvents::Impl
{
	std::function<void()> m_CaptureLost;
#if defined(_WIN32)
	HWND m_Handle = nullptr;
	static LRESULT CALLBACK Procedure(HWND handle, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR data)
	{
		auto& self = *reinterpret_cast<Impl*>(data);
		if ((message == WM_CAPTURECHANGED && reinterpret_cast<HWND>(lparam) != handle) || message == WM_CANCELMODE)
			self.m_CaptureLost();
		return DefSubclassProc(handle, message, wparam, lparam);
	}
#endif
};
WindowEvents::WindowEvents(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl)) {}
WindowEvents::~WindowEvents()
{
#if defined(_WIN32)
	if (m_Impl->m_Handle) RemoveWindowSubclass(m_Impl->m_Handle, Impl::Procedure, 2);
#endif
}
Result<std::unique_ptr<WindowEvents>> WindowEvents::Create(SDL_Window* window, std::function<void()> captureLost)
{
	auto impl = std::make_unique<Impl>();
	impl->m_CaptureLost = std::move(captureLost);
#if defined(_WIN32)
	impl->m_Handle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
	if (!impl->m_Handle || !SetWindowSubclass(impl->m_Handle, Impl::Procedure, 2, reinterpret_cast<DWORD_PTR>(impl.get())))
		return std::unexpected(Error{ErrorCode::WindowCreation, "Installing window event handling failed."});
#else
	(void)window;
#endif
	return std::unique_ptr<WindowEvents>(new WindowEvents(std::move(impl)));
}
}
