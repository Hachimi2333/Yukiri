#include "WindowChrome.hpp"
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <SDL3/SDL.h>
#include <utility>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#undef DrawText

namespace Yukiri::Detail
{
struct WindowChrome::Impl
{
	Window* m_Owner = nullptr;
	HWND m_Handle = nullptr;
	int m_Hover = 0;
	int m_Pressed = 0;
	float Scale() const { return static_cast<float>(GetDpiForWindow(m_Handle)) / 96.0f; }
	int ButtonAt(POINT point) const
	{
		RECT rectangle{};
		GetClientRect(m_Handle, &rectangle);
		const float scale = Scale();
		if (point.y < 0 || static_cast<float>(point.y) >= 40 * scale) return 0;
		const float right = static_cast<float>(rectangle.right - point.x) / scale;
		return right >= 0 && right < 46 ? HTCLOSE : right < 92 && right >= 0 ? HTMAXBUTTON : right < 138 && right >= 0 ? HTMINBUTTON : 0;
	}
	LRESULT HitTest(LPARAM parameter) const
	{
		POINT point{GET_X_LPARAM(parameter), GET_Y_LPARAM(parameter)};
		ScreenToClient(m_Handle, &point);
		RECT rectangle{};
		GetClientRect(m_Handle, &rectangle);
		if (!IsZoomed(m_Handle))
		{
			const int border = std::max(4, static_cast<int>(6 * Scale()));
			const bool left = point.x < border;
			const bool right = point.x >= rectangle.right - border;
			const bool top = point.y < border;
			const bool bottom = point.y >= rectangle.bottom - border;
			if (top && left) return HTTOPLEFT;
			if (top && right) return HTTOPRIGHT;
			if (bottom && left) return HTBOTTOMLEFT;
			if (bottom && right) return HTBOTTOMRIGHT;
			if (left) return HTLEFT;
			if (right) return HTRIGHT;
			if (top) return HTTOP;
			if (bottom) return HTBOTTOM;
		}
		if (const int button = ButtonAt(point)) return button;
		return static_cast<float>(point.y) < 40 * Scale() ? HTCAPTION : HTCLIENT;
	}
	static LRESULT CALLBACK Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR reference)
	{
		auto& self = *reinterpret_cast<Impl*>(reference);
		switch (message)
		{
		case WM_NCCALCSIZE:
			if (wParam && IsZoomed(handle))
			{
				MONITORINFO monitor{};
				monitor.cbSize = sizeof(monitor);
				if (GetMonitorInfoW(MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST), &monitor))
					reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam)->rgrc[0] = monitor.rcWork;
			}
			return 0;
		case WM_NCHITTEST:
		{
			LRESULT result = 0;
			DwmDefWindowProc(handle, message, wParam, lParam, &result);
			return self.HitTest(lParam);
		}
		case WM_GETMINMAXINFO:
		{
			const auto result = DefSubclassProc(handle, message, wParam, lParam);
			MONITORINFO monitor{};
			monitor.cbSize = sizeof(monitor);
			if (GetMonitorInfoW(MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST), &monitor))
			{
				auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
				limits->ptMaxPosition = {monitor.rcWork.left - monitor.rcMonitor.left, monitor.rcWork.top - monitor.rcMonitor.top};
				limits->ptMaxSize = {monitor.rcWork.right - monitor.rcWork.left, monitor.rcWork.bottom - monitor.rcWork.top};
			}
			return result;
		}
		case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
		{
			POINT point{};
			GetCursorPos(&point);
			ScreenToClient(handle, &point);
			const int hover = self.ButtonAt(point);
			if (hover != self.m_Hover) { self.m_Hover = hover; self.m_Owner->Invalidate(); }
			if (message == WM_NCMOUSEMOVE)
			{
				TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE | TME_NONCLIENT, handle, 0};
				TrackMouseEvent(&track);
			}
			break;
		}
		case WM_NCMOUSELEAVE:
			self.m_Hover = 0;
			self.m_Owner->Invalidate();
			break;
		case WM_NCLBUTTONDOWN:
			if (wParam == HTCLOSE || wParam == HTMAXBUTTON || wParam == HTMINBUTTON)
			{
				self.m_Pressed = static_cast<int>(wParam);
				SetCapture(handle);
				self.m_Owner->Invalidate();
				return 0;
			}
			break;
		case WM_LBUTTONUP:
		case WM_NCLBUTTONUP:
			if (self.m_Pressed)
			{
				POINT point{};
				GetCursorPos(&point);
				ScreenToClient(handle, &point);
				const int pressed = std::exchange(self.m_Pressed, 0);
				ReleaseCapture();
				self.m_Owner->Invalidate();
				if (self.ButtonAt(point) == pressed)
				{
					const WPARAM command = pressed == HTCLOSE ? SC_CLOSE : pressed == HTMINBUTTON ? SC_MINIMIZE :
						IsZoomed(handle) ? SC_RESTORE : SC_MAXIMIZE;
					PostMessageW(handle, WM_SYSCOMMAND, command, 0);
				}
				return 0;
			}
			break;
		case WM_CAPTURECHANGED:
			self.m_Pressed = 0;
			self.m_Owner->Invalidate();
			break;
		case WM_NCRBUTTONUP:
			if (wParam == HTCAPTION)
			{
				POINT point{};
				GetCursorPos(&point);
				const auto command = TrackPopupMenu(GetSystemMenu(handle, FALSE), TPM_RETURNCMD | TPM_RIGHTBUTTON,
					point.x, point.y, 0, handle, nullptr);
				if (command) PostMessageW(handle, WM_SYSCOMMAND, static_cast<WPARAM>(command), 0);
				return 0;
			}
			break;
		case WM_SIZE: case WM_DPICHANGED: self.m_Owner->Invalidate(); break;
		default: break;
		}
		return DefSubclassProc(handle, message, wParam, lParam);
	}
};

WindowChrome::WindowChrome(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl)) {}
Result<std::unique_ptr<WindowChrome>> WindowChrome::Create(Window& owner, SDL_Window* native)
{
	auto impl = std::make_unique<Impl>();
	impl->m_Owner = &owner;
	impl->m_Handle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(native), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
	if (!impl->m_Handle || !SetWindowSubclass(impl->m_Handle, Impl::Procedure, 1, reinterpret_cast<DWORD_PTR>(impl.get())))
		return std::unexpected(Error{ErrorCode::WindowCreation, "Installing the Windows custom frame failed."});
	auto chrome = std::unique_ptr<WindowChrome>(new WindowChrome(std::move(impl)));
	const auto handle = chrome->m_Impl->m_Handle;
	const auto style = GetWindowLongPtrW(handle, GWL_STYLE);
	SetWindowLongPtrW(handle, GWL_STYLE, style | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
	const MARGINS margins{0, 0, 1, 0};
	DwmExtendFrameIntoClientArea(handle, &margins);
	SetWindowPos(handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	return chrome;
}
WindowChrome::~WindowChrome()
{
	if (m_Impl->m_Handle) RemoveWindowSubclass(m_Impl->m_Handle, Impl::Procedure, 1);
}
void WindowChrome::Paint(Canvas& canvas)
{
	const auto size = m_Impl->m_Owner->GetSize();
	const auto& theme = m_Impl->m_Owner->GetTheme();
	canvas.FillRectangle({0, 0, size.m_Width, 40}, theme.m_Surface);
	canvas.FillRectangle({0, 39, size.m_Width, 1}, theme.m_Border);
	canvas.PushClip({12, 0, std::max(0.0f, size.m_Width - 162), 40});
	canvas.DrawText(m_Impl->m_Owner->GetConfig().m_Title, {12, 10}, theme.m_Text, 13);
	canvas.PopClip();
	const int buttons[] = {HTMINBUTTON, HTMAXBUTTON, HTCLOSE};
	for (int i = 0; i < 3; ++i)
	{
		const float x = size.m_Width - static_cast<float>(3 - i) * 46;
		const bool hover = m_Impl->m_Hover == buttons[i];
		if (hover) canvas.FillRectangle({x, 0, 46, 39}, buttons[i] == HTCLOSE ? Color::FromHex(0xD84048) :
			m_Impl->m_Pressed == buttons[i] ? theme.m_Pressed : theme.m_Hover);
		const auto color = hover && buttons[i] == HTCLOSE ? Color::FromHex(0xFFFFFF) : theme.m_Text;
		if (buttons[i] == HTMINBUTTON) canvas.DrawLine({x + 18, 22}, {x + 28, 22}, color, 1);
		else if (buttons[i] == HTMAXBUTTON)
		{
			if (IsZoomed(m_Impl->m_Handle))
			{
				canvas.StrokeRectangle({x + 20, 14, 9, 8}, color, 1);
				canvas.FillRectangle({x + 17, 17, 9, 8}, theme.m_Surface);
				canvas.StrokeRectangle({x + 17, 17, 9, 8}, color, 1);
			}
			else canvas.StrokeRectangle({x + 18, 15, 10, 9}, color, 1);
		}
		else
		{
			canvas.DrawLine({x + 18, 15}, {x + 28, 25}, color, 1);
			canvas.DrawLine({x + 28, 15}, {x + 18, 25}, color, 1);
		}
	}
}
}
#else
namespace Yukiri::Detail
{
struct WindowChrome::Impl {};
WindowChrome::WindowChrome(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl)) {}
WindowChrome::~WindowChrome() = default;
Result<std::unique_ptr<WindowChrome>> WindowChrome::Create(Window&, SDL_Window*)
{
	return std::unexpected(Error{ErrorCode::WindowCreation, "Custom decorations are currently implemented on Windows only."});
}
void WindowChrome::Paint(Canvas&) {}
}
#endif
