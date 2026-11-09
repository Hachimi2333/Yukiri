#include "WindowsAccessibility.hpp"
#if defined(_WIN32)
#include "AutomationContext.hpp"
#include <SDL3/SDL.h>
#include <commctrl.h>

namespace Yukiri::Detail
{
struct WindowsAccessibility::Impl
{
	HWND m_Handle = nullptr;
	std::shared_ptr<AutomationContext> m_Context;
	ComPtr<IRawElementProviderSimple> m_Root;
	static LRESULT CALLBACK Procedure(HWND handle, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR data)
	{
		auto& self = *reinterpret_cast<Impl*>(data);
		if (message == WM_GETOBJECT && static_cast<LONG>(lparam) == UiaRootObjectId)
		{
			try { return UiaReturnRawElementProvider(handle, wparam, lparam, self.m_Root.Get()); }
			catch (...) { return 0; }
		}
		return DefSubclassProc(handle, message, wparam, lparam);
	}
};
WindowsAccessibility::WindowsAccessibility(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl)) {}
Result<std::unique_ptr<WindowsAccessibility>> WindowsAccessibility::Create(SDL_Window* window, WidgetTree& tree)
{
	auto impl = std::make_unique<Impl>();
	impl->m_Handle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
	if (!impl->m_Handle) return std::unexpected(Error{ErrorCode::WindowCreation, "The accessibility host window is unavailable."});
	impl->m_Context = std::make_shared<AutomationContext>(tree, impl->m_Handle);
	impl->m_Root = impl->m_Context->Get(impl->m_Context->m_Tree->Root());
	if (!SetWindowSubclass(impl->m_Handle, Impl::Procedure, 3, reinterpret_cast<DWORD_PTR>(impl.get()))) return std::unexpected(Error{ErrorCode::WindowCreation, "Installing the UI Automation provider failed."});
	if (UiaClientsAreListening()) UiaRaiseAutomationEvent(impl->m_Root.Get(), UIA_Window_WindowOpenedEventId);
	return std::unique_ptr<WindowsAccessibility>(new WindowsAccessibility(std::move(impl)));
}
WindowsAccessibility::~WindowsAccessibility()
{
	RemoveWindowSubclass(m_Impl->m_Handle, Impl::Procedure, 3);
	if (UiaClientsAreListening()) UiaRaiseAutomationEvent(m_Impl->m_Root.Get(), UIA_Window_WindowClosedEventId);
	m_Impl->m_Context->Shutdown();
}
void WindowsAccessibility::Flush()
{
	try { m_Impl->m_Context->m_Tree->EnsureLayout(); m_Impl->m_Context->Flush(); }
	catch (...) { /* Accessibility notifications must not interrupt presentation. */ }
}
}
#else
namespace Yukiri::Detail
{
struct WindowsAccessibility::Impl {};
WindowsAccessibility::WindowsAccessibility(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl)) {}
WindowsAccessibility::~WindowsAccessibility() = default;
Result<std::unique_ptr<WindowsAccessibility>> WindowsAccessibility::Create(SDL_Window*, WidgetTree&) { return std::unique_ptr<WindowsAccessibility>{}; }
void WindowsAccessibility::Flush() {}
}
#endif
