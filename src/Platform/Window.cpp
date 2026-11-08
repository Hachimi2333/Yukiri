#include <Yukiri/Platform/Window.hpp>
#include "WindowState.hpp"
#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri
{
Window::Window(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl))
{
	m_Impl->m_Owner = this;
	m_Impl->m_Tree.m_Window = this;
	m_Impl->m_Tree.m_EndTextInput = [this] { StopTextInput(); };
	m_Impl->m_Tree.m_PointerCapture = [](bool capture) { SDL_CaptureMouse(capture); };
	SetTheme(m_Impl->m_Config.m_Theme);
}

Window::~Window()
{
	m_Impl->m_Tree.SetRoot(nullptr);
	m_Impl->m_Renderer.reset();
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

void Window::SetContent(std::unique_ptr<Widget> content) { m_Impl->m_Tree.SetRoot(std::move(content)); Invalidate(); }
Widget* Window::GetContent() const noexcept { return m_Impl->m_Tree.m_Root.get(); }
const Theme& Window::GetTheme() const noexcept { return m_Impl->m_Tree.m_Theme; }
void Window::SetTheme(ThemeMode mode)
{
	m_Impl->m_Config.m_Theme = mode;
	m_Impl->m_Tree.m_Theme = mode == ThemeMode::Dark ? Theme::Dark() : Theme::Light();
	m_Impl->m_Tree.m_LayoutDirty = true;
	Invalidate();
}

Result<std::string> Window::GetClipboard() const
{
	char* value = SDL_GetClipboardText();
	if (!value) return std::unexpected(Error{ErrorCode::ResourceLoading, SDL_GetError()});
	std::string text(value);
	SDL_free(value);
	return text;
}
Result<void> Window::SetClipboard(std::string_view text)
{
	const std::string owned(text);
	if (!SDL_SetClipboardText(owned.c_str())) return std::unexpected(Error{ErrorCode::ResourceLoading, SDL_GetError()});
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

Result<void> Window::Update()
{
	if (!m_Impl->m_Dirty && !m_Impl->m_Tree.m_Dirty) return {};
	Detail::DrawList list;
	const auto size = GetSize();
	m_Impl->m_Tree.Layout(size);
	Canvas canvas(list, size);
	m_Impl->m_Tree.Paint(canvas);
	auto result = m_Impl->m_Renderer->Draw(list, size, GetTheme().m_Background);
	if (!result) return result;
	m_Impl->m_Dirty = false;
	m_Impl->m_Tree.m_Dirty = false;
	return {};
}
}
