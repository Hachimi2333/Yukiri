#include <Yukiri/Core/Command.hpp>
#include <utility>

namespace Yukiri
{
bool KeyGesture::Matches(const KeyEvent& event) const noexcept
{
	return m_Key != Key::Unknown && !event.m_Modifiers.m_AltGraph && m_Key == event.m_Key && m_Modifiers == event.m_Modifiers;
}
Command::Command(std::string text) : m_Text(std::move(text)) {}
const std::string& Command::GetText() const noexcept { return m_Text; }
bool Command::IsEnabled() const noexcept { return m_Enabled; }
bool Command::IsCheckable() const noexcept { return m_Checkable; }
bool Command::IsChecked() const noexcept { return m_Checked; }
const std::optional<KeyGesture>& Command::GetShortcut() const noexcept { return m_Shortcut; }
void Command::SetText(std::string text) { if (m_Text != text) { m_Text = std::move(text); m_Changed.Emit(); } }
void Command::SetEnabled(bool enabled) { if (m_Enabled != enabled) { m_Enabled = enabled; m_Changed.Emit(); } }
void Command::SetCheckable(bool checkable) { if (m_Checkable != checkable) { m_Checkable = checkable; m_Changed.Emit(); } }
void Command::SetChecked(bool checked) { if (m_Checked != checked) { m_Checked = checked; m_Changed.Emit(); } }
void Command::SetShortcut(std::optional<KeyGesture> shortcut) { if (m_Shortcut != shortcut) { m_Shortcut = shortcut; m_Changed.Emit(); } }
void Command::SetAllowRepeat(bool allowRepeat) { m_AllowRepeat = allowRepeat; }
bool Command::Execute()
{
	if (!m_Enabled) return false;
	m_Executed.Emit();
	return true;
}
bool Command::TryExecute(KeyEvent& event)
{
	if (!m_Shortcut || !m_Enabled || !m_Shortcut->Matches(event)) return false;
	event.m_Handled = true;
	if (!event.m_Repeat || m_AllowRepeat) Execute();
	return true;
}
}
