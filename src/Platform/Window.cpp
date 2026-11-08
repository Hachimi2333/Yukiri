#include <Yukiri/Platform/Window.hpp>
#include "WindowState.hpp"
#include <Yukiri/Render/Canvas.hpp>
#include "Appearance.hpp"
#include <cmath>
#include <limits>

namespace Yukiri
{
Window::Window(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl))
{
	m_Impl->m_Owner = this;
	m_Impl->m_Tree.m_Window = this;
	m_Impl->m_Tree.m_EndTextInput = [this] { StopTextInput(); };
	m_Impl->m_Tree.m_PointerCapture = [](bool capture) { SDL_CaptureMouse(capture); };
	m_Impl->m_Tree.m_MeasureText = [this](std::string_view text, float size) { return MeasureText(text, size); };
	m_Impl->m_Cursors = {SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT), SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT)};
	m_Impl->m_Statistics.m_Enabled = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "YUKIRI_FRAME_STATS") != nullptr;
	m_Impl->RefreshRate();
	SetTheme(m_Impl->m_Config.m_Theme);
}

Window::~Window()
{
	m_Impl->m_Statistics.Print(SDL_GetWindowID(m_Impl->m_Native), m_Impl->m_Pacer.GetInterval());
	m_Impl->m_Tree.SetRoot(nullptr);
	m_Impl->m_Chrome.reset();
	m_Impl->m_Renderer.reset();
	for (auto* cursor : m_Impl->m_Cursors) if (cursor) SDL_DestroyCursor(cursor);
	if (m_Impl->m_Native)
		SDL_DestroyWindow(m_Impl->m_Native);
}

void Window::SetTitle(std::string_view title)
{
	m_Impl->m_Config.m_Title = title;
	SDL_SetWindowTitle(m_Impl->m_Native, m_Impl->m_Config.m_Title.c_str());
}

void Window::Close() { m_Impl->m_Open = false; }
void Window::Invalidate() { m_Impl->m_Dirty = m_Impl->m_Tree.m_Dirty = true; }
bool Window::IsOpen() const noexcept { return m_Impl->m_Open; }
const WindowConfig& Window::GetConfig() const noexcept { return m_Impl->m_Config; }

Size Window::GetSize() const
{
	int width = 0;
	int height = 0;
	SDL_GetWindowSize(m_Impl->m_Native, &width, &height);
	const float scale = GetScale();
	return {static_cast<float>(width) / scale, static_cast<float>(height) / scale};
}

float Window::GetScale() const { return std::max(1.0f, SDL_GetWindowDisplayScale(m_Impl->m_Native)); }
Rect Window::GetContentBounds() const
{
	const auto size = GetSize();
	const float titleHeight = m_Impl->m_Chrome ? m_Impl->m_Chrome->GetHeight() : 0;
	return {0, titleHeight, size.m_Width, std::max(0.0f, size.m_Height - titleHeight)};
}

void Window::SetContent(std::unique_ptr<Widget> content) { m_Impl->m_Tree.SetRoot(std::move(content)); Invalidate(); }
Widget* Window::GetContent() const noexcept { return m_Impl->m_Tree.m_Root.get(); }
const Theme& Window::GetTheme() const noexcept { return m_Impl->m_Tree.m_Theme; }
void Window::SetTheme(ThemeMode mode)
{
	m_Impl->m_Config.m_Theme = mode;
	const bool dark = mode == ThemeMode::Dark || (mode == ThemeMode::System && Detail::IsSystemDark());
	m_Impl->m_Tree.m_Theme = dark ? Theme::Dark() : Theme::Light();
	m_Impl->m_Tree.m_Theme.m_Motion = m_Impl->m_Motion;
	Detail::ApplyAppearance(m_Impl->m_Native, dark);
	m_Impl->m_Tree.m_LayoutDirty = true;
	Invalidate();
}

Result<std::string> Window::GetClipboard() const
{
	char* value = SDL_GetClipboardText();
	if (!value) return std::unexpected(Error{ErrorCode::PlatformOperation, SDL_GetError()});
	std::string text(value);
	SDL_free(value);
	return text;
}
Result<void> Window::SetClipboard(std::string_view text)
{
	const std::string owned(text);
	if (!SDL_SetClipboardText(owned.c_str())) return std::unexpected(Error{ErrorCode::PlatformOperation, SDL_GetError()});
	return {};
}
void Window::StartTextInput(Rect caret)
{
	const float scale = GetScale();
	SDL_Rect rectangle{static_cast<int>(caret.m_X * scale), static_cast<int>(caret.m_Y * scale),
		std::max(1, static_cast<int>(caret.m_Width * scale)), std::max(1, static_cast<int>(caret.m_Height * scale))};
	if (!SDL_TextInputActive(m_Impl->m_Native)) SDL_StartTextInput(m_Impl->m_Native);
	SDL_SetTextInputArea(m_Impl->m_Native, &rectangle, 0);
}
void Window::StopTextInput() { SDL_StopTextInput(m_Impl->m_Native); }
namespace
{
Result<void> PlatformOperation(bool success)
{
	if (!success) return std::unexpected(Error{ErrorCode::PlatformOperation, SDL_GetError()});
	return {};
}
}
Result<void> Window::SetSize(Size size)
{
	const double width = static_cast<double>(size.m_Width) * GetScale();
	const double height = static_cast<double>(size.m_Height) * GetScale();
	if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0 ||
		width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max())
		return std::unexpected(Error{ErrorCode::InvalidArgument, "Requested window size is invalid."});
	Invalidate();
	return PlatformOperation(SDL_SetWindowSize(m_Impl->m_Native, static_cast<int>(std::lround(width)), static_cast<int>(std::lround(height))));
}
Result<void> Window::Minimize() { return PlatformOperation(SDL_MinimizeWindow(m_Impl->m_Native)); }
Result<void> Window::Maximize() { return PlatformOperation(SDL_MaximizeWindow(m_Impl->m_Native)); }
Result<void> Window::Restore() { return PlatformOperation(SDL_RestoreWindow(m_Impl->m_Native)); }

void Window::Impl::RefreshRate()
{
	const auto* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(m_Native));
	if (mode) m_Pacer.SetRefreshRate(mode->refresh_rate_numerator, mode->refresh_rate_denominator, mode->refresh_rate);
	else m_Pacer.SetRefreshRate(0, 0);
}
Detail::FramePacer::Time Window::Impl::NextUpdate(Detail::FramePacer::Time now) const
{
	if (!m_Open) return now;
	if (SDL_GetWindowFlags(m_Native) & SDL_WINDOW_MINIMIZED) return Detail::FramePacer::Time::max();
	if (m_Dirty || m_Tree.m_Dirty || m_Tree.m_Animating) return m_Pacer.GetDeadline();
	if (m_Tree.m_Focus && m_Tree.m_Focus->AcceptsTextInput())
	{
		const auto ticks = SDL_GetTicks();
		return now + std::chrono::milliseconds(500 - ticks % 500);
	}
	return Detail::FramePacer::Time::max();
}
Result<void> Window::Update(bool paced)
{
	using Clock = Detail::FramePacer::Clock;
	const auto start = Clock::now();
	if (SDL_GetWindowFlags(m_Impl->m_Native) & SDL_WINDOW_MINIMIZED)
	{
		m_Impl->m_Dirty = m_Impl->m_Tree.m_Dirty = m_Impl->m_Tree.m_Animating = false;
		m_Impl->m_Pacer.Reset();
		return {};
	}
	const auto blink = SDL_GetTicks() / 500;
	if (m_Impl->m_Tree.m_Focus && m_Impl->m_Tree.m_Focus->AcceptsTextInput() && blink != m_Impl->m_BlinkTick)
		m_Impl->m_Tree.m_Dirty = true;
	m_Impl->m_BlinkTick = blink;
	if (!m_Impl->m_Dirty && !m_Impl->m_Tree.m_Dirty && !m_Impl->m_Tree.m_Animating) return {};
	if (paced && !m_Impl->m_Pacer.IsDue(start)) return {};
	auto ready = m_Impl->m_Renderer->TryBeginFrame();
	if (!ready) return std::unexpected(ready.error());
	if (!*ready)
	{
		m_Impl->m_Statistics.Busy();
		m_Impl->m_Pacer.Retry(start);
		return {};
	}
	m_Impl->m_Tree.Tick(start);
	m_Impl->m_Dirty = false;
	m_Impl->m_Tree.m_Dirty = false;
	Detail::DrawList list;
	const auto size = GetSize();
	const auto content = GetContentBounds();
	m_Impl->m_Tree.Layout({content.m_Width, content.m_Height}, {content.m_X, content.m_Y});
	Canvas canvas(list, size, m_Impl->m_Text, GetScale());
	m_Impl->m_Tree.Paint(canvas);
	if (m_Impl->m_Chrome) m_Impl->m_Chrome->Paint(canvas);
	if (list.m_Error) return std::unexpected(*list.m_Error);
	const auto beforeDraw = Clock::now();
	auto result = m_Impl->m_Renderer->Draw(list, size, GetTheme().m_Background);
	if (!result) return result;
	m_Impl->m_Statistics.Record(start, std::chrono::duration<double, std::milli>(beforeDraw - start).count(),
		m_Impl->m_Renderer->GetRecordMilliseconds(), m_Impl->m_Renderer->GetPresentMilliseconds());
	m_Impl->m_Pacer.Submitted(start);
	if (m_Impl->m_Renderer->NeedsRetry()) m_Impl->m_Dirty = true;
	return {};
}
}
