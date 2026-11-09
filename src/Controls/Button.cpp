#include <Yukiri/Controls/Button.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/Unicode.hpp"
#include "Text/TextLayout.hpp"

namespace Yukiri
{
Button::Button(std::string text) : m_Text(Detail::SanitizeUtf8(text, true)) { SetFocusable(true); SetCursor(Cursor::Hand); }
void Button::SetText(std::string_view text)
{
	auto sanitized = Detail::SanitizeUtf8(text, true);
	if (m_Text == sanitized) return;
	m_Text = std::move(sanitized);
	InvalidateLayout();
}
const std::string& Button::GetText() const noexcept { return m_Text; }
void Button::SetAccent(bool accent) { m_Accent = accent; InvalidatePaint(); }
void Button::SetIcon(Image icon) { if (m_Icon == icon) return; m_Icon = std::move(icon); InvalidateLayout(); }
const Image& Button::GetIcon() const noexcept { return m_Icon; }
std::shared_ptr<Command> Button::GetCommand() const noexcept { return m_Command; }
void Button::SetCommand(std::shared_ptr<Command> command)
{
	m_CommandChanged.Disconnect();
	m_Command = std::move(command);
	if (!m_Command) return;
	auto synchronize = [this, lifetime = ObserveLifetime()]
	{
		auto current = m_Command;
		SetText(current->GetText());
		SetIcon(current->GetIcon());
		InvalidatePaint();
		OnCommandChecked(current->IsCheckable(), current->IsChecked());
		if (!lifetime.expired() && m_Command == current) SetEnabled(current->IsEnabled());
	};
	m_CommandChanged = m_Command->m_Changed.Connect(synchronize);
	synchronize();
}
Size Button::OnMeasure(Size) { return {std::max(72.0f, MeasureText(m_Text).m_Width + 28 + (m_Icon ? 24 : 0)), GetTheme().m_ControlHeight}; }
void Button::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	m_Pressed = m_Armed = true;
	CapturePointer();
	InvalidatePaint();
	event.m_Handled = true;
}
void Button::OnPointerMove(PointerEvent& event)
{
	if (!m_Pressed) return;
	m_Armed = GetBounds().Contains(event.m_Position);
	InvalidatePaint();
	event.m_Handled = true;
}
void Button::OnPointerUp(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left || !m_Pressed) return;
	const bool activate = GetBounds().Contains(event.m_Position);
	m_Pressed = m_Armed = false;
	ReleasePointer();
	InvalidatePaint();
	event.m_Handled = true;
	if (activate) Activate();
}
void Button::OnKeyDown(KeyEvent& event)
{
	if (event.m_Key == Key::Space)
	{
		m_Pressed = m_Armed = m_KeyboardPressed = true;
		InvalidatePaint();
		event.m_Handled = true;
	}
	else if (event.m_Key == Key::Enter && !event.m_Repeat)
	{
		event.m_Handled = true;
		Activate();
	}
}
void Button::OnKeyUp(KeyEvent& event)
{
	if (event.m_Key != Key::Space || !m_KeyboardPressed) return;
	m_Pressed = m_Armed = m_KeyboardPressed = false;
	event.m_Handled = true;
	InvalidatePaint();
	Activate();
}
void Button::OnPointerCaptureLost() { m_Pressed = m_Armed = m_KeyboardPressed = false; InvalidatePaint(); }
void Button::OnFocusChanged(bool focused)
{
	if (!focused) { m_Pressed = m_Armed = m_KeyboardPressed = false; ReleasePointer(); }
	InvalidatePaint();
}
void Button::Activate()
{
	const auto lifetime = ObserveLifetime();
	auto command = m_Command;
	if (command && !command->Execute()) return;
	if (!lifetime.expired()) m_Clicked.Emit();
}
void Button::OnCommandChecked(bool, bool) {}
void Button::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const auto size = GetActualSize();
	const bool checked = m_Command && m_Command->IsCheckable() && m_Command->IsChecked();
	const Color resting = GetStyle().m_Background.value_or(m_Accent && IsEnabled() ? theme.m_Accent : checked ? theme.m_Selection : theme.m_Surface);
	const Color hover = GetStyle().m_Hover.value_or(m_Accent ? theme.m_AccentHover : theme.m_Hover);
	const auto background = InteractiveColor(resting, hover, GetStyle().m_Pressed.value_or(m_Accent ? theme.m_AccentHover : theme.m_Pressed), m_Pressed && m_Armed);
	canvas.FillRectangle(GetLocalBounds(), background, GetRadius());
	canvas.StrokeRectangle(GetLocalBounds(), GetStyle().m_Border.value_or(m_Accent ? background : theme.m_Border), 1, GetRadius());
	const float iconWidth = m_Icon ? 16.0f : 0;
	const float gap = m_Icon && !m_Text.empty() ? 8.0f : 0;
	const auto text = Detail::FitText(m_Text, std::max(0.0f, size.m_Width - 20 - iconWidth - gap),
		[&](std::string_view value) { return canvas.MeasureText(value, GetFontSize()); });
	const auto measured = canvas.MeasureText(text, GetFontSize());
	const float left = (size.m_Width - measured.m_Width - iconWidth - gap) * 0.5f;
	if (m_Icon) canvas.DrawImage(m_Icon, {left, (size.m_Height - 16) * 0.5f, 16, 16}, IsEnabled() ? 1.0f : 0.5f);
	canvas.DrawText(text, {left + iconWidth + gap, (size.m_Height - measured.m_Height) * 0.5f},
		m_Accent && IsEnabled() ? theme.m_OnAccent : GetForeground(), GetFontSize());
	PaintFocus(canvas);
}
}
