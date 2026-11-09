#pragma once

#include <Yukiri/Core/Signal.hpp>
#include <Yukiri/UI/Input.hpp>
#include <optional>
#include <string>

namespace Yukiri
{
struct KeyGesture
{
	Key m_Key = Key::Unknown;
	Modifiers m_Modifiers;
	[[nodiscard]] bool Matches(const KeyEvent& event) const noexcept;
	bool operator==(const KeyGesture&) const = default;
};

class Command
{
public:
	explicit Command(std::string text = {});
	void SetText(std::string text);
	void SetEnabled(bool enabled);
	void SetCheckable(bool checkable);
	void SetChecked(bool checked);
	void SetShortcut(std::optional<KeyGesture> shortcut);
	void SetAllowRepeat(bool allowRepeat);
	[[nodiscard]] const std::string& GetText() const noexcept;
	[[nodiscard]] bool IsEnabled() const noexcept;
	[[nodiscard]] bool IsCheckable() const noexcept;
	[[nodiscard]] bool IsChecked() const noexcept;
	[[nodiscard]] const std::optional<KeyGesture>& GetShortcut() const noexcept;
	bool Execute();
	bool TryExecute(KeyEvent& event);
	Signal<> m_Executed;
	Signal<> m_Changed;
private:
	std::string m_Text;
	std::optional<KeyGesture> m_Shortcut;
	bool m_Enabled = true;
	bool m_Checkable = false;
	bool m_Checked = false;
	bool m_AllowRepeat = false;
};
}
