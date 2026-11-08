#include <Yukiri/Platform/Window.hpp>
#include "WindowState.hpp"

namespace Yukiri
{
Window::Window(std::unique_ptr<Impl> impl) : m_Impl(std::move(impl)) {}

Window::~Window()
{
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
void Window::Invalidate() { m_Impl->m_Dirty = true; }
bool Window::IsOpen() const noexcept { return m_Impl->m_Open; }
const WindowConfig& Window::GetConfig() const noexcept { return m_Impl->m_Config; }

Size Window::GetSize() const
{
	int width = 0;
	int height = 0;
	SDL_GetWindowSize(m_Impl->m_Native, &width, &height);
	return {static_cast<float>(width), static_cast<float>(height)};
}

float Window::GetScale() const { return SDL_GetWindowDisplayScale(m_Impl->m_Native); }

Result<void> Window::Update()
{
	if (!m_Impl->m_Dirty) return {};
	Detail::DrawList list;
	auto result = m_Impl->m_Renderer->Draw(list, GetSize(), Color::FromHex(0xF6F7FA));
	if (!result) return result;
	m_Impl->m_Dirty = false;
	return {};
}
}
