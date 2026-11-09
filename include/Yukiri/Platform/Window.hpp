#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Core/Geometry.hpp>
#include <Yukiri/Core/Signal.hpp>
#include <Yukiri/Platform/Config.hpp>
#include <Yukiri/Styling/Theme.hpp>

#include <memory>
#include <string_view>
#include <filesystem>
#include <vector>

namespace Yukiri
{
class Application;
class Widget;
class Command;

struct CloseRequestEvent
{
	bool m_Cancel = false;
};

class Window final
{
public:
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	void SetTitle(std::string_view title);
	void Close();
	[[nodiscard]] Result<void> Show();
	[[nodiscard]] Result<void> Hide();
	[[nodiscard]] Result<void> Activate();
	[[nodiscard]] Result<void> SetOwner(Window* owner);
	[[nodiscard]] Window* GetOwner() const noexcept;
	void AddCommand(std::shared_ptr<Command> command);
	void RemoveCommand(const Command& command);
	void Invalidate();
	void SetContent(std::unique_ptr<Widget> content);
	[[nodiscard]] Widget* GetContent() const noexcept;
	void SetTheme(ThemeMode mode);
	[[nodiscard]] const Theme& GetTheme() const noexcept;
	[[nodiscard]] Result<std::string> GetClipboard() const;
	[[nodiscard]] Result<void> SetClipboard(std::string_view text);
	void StartTextInput(Rect caret);
	void StopTextInput();
	[[nodiscard]] Result<void> SetSize(Size size);
	[[nodiscard]] Result<void> Minimize();
	[[nodiscard]] Result<void> Maximize();
	[[nodiscard]] Result<void> Restore();
	[[nodiscard]] Size MeasureText(std::string_view text, float fontSize = 14) const;
	[[nodiscard]] float GetTextCaretX(std::string_view text, std::size_t offset, float fontSize = 14) const;
	[[nodiscard]] std::size_t HitTestText(std::string_view text, float x, float fontSize = 14) const;
	[[nodiscard]] bool IsOpen() const noexcept;
	[[nodiscard]] Size GetSize() const;
	[[nodiscard]] Rect GetContentBounds() const;
	[[nodiscard]] float GetScale() const;
	[[nodiscard]] const WindowConfig& GetConfig() const noexcept;
	Signal<> m_Closed;
	Signal<CloseRequestEvent&> m_CloseRequested;
	Signal<Size> m_SizeChanged;
	Signal<> m_StateChanged;
	Signal<std::vector<std::filesystem::path>> m_FilesDropped;

private:
	friend class Application;
	struct Impl;
	explicit Window(std::unique_ptr<Impl> impl);
	[[nodiscard]] Result<void> Update(bool paced);
	std::unique_ptr<Impl> m_Impl;
};
}
