#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Core/Geometry.hpp>
#include <Yukiri/Platform/Config.hpp>
#include <Yukiri/Styling/Theme.hpp>

#include <memory>
#include <string_view>

namespace Yukiri
{
class Application;
class Widget;

class Window final
{
public:
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	void SetTitle(std::string_view title);
	void Close();
	void Invalidate();
	void SetContent(std::unique_ptr<Widget> content);
	[[nodiscard]] Widget* GetContent() const noexcept;
	void SetTheme(ThemeMode mode);
	[[nodiscard]] const Theme& GetTheme() const noexcept;
	[[nodiscard]] Result<std::string> GetClipboard() const;
	[[nodiscard]] Result<void> SetClipboard(std::string_view text);
	void StartTextInput(Rect caret);
	void StopTextInput();
	[[nodiscard]] bool IsOpen() const noexcept;
	[[nodiscard]] Size GetSize() const;
	[[nodiscard]] float GetScale() const;
	[[nodiscard]] const WindowConfig& GetConfig() const noexcept;

private:
	friend class Application;
	struct Impl;
	explicit Window(std::unique_ptr<Impl> impl);
	[[nodiscard]] Result<void> Update();
	std::unique_ptr<Impl> m_Impl;
};
}
