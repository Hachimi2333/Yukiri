#include <Yukiri/Controls/Button.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/Unicode.hpp"

namespace Yukiri
{
Button::Button(std::string text) : m_Text(Detail::SanitizeUtf8(text, true)) { SetFocusable(true); SetCursor(Cursor::Hand); }
void Button::SetText(std::string_view text) { m_Text = Detail::SanitizeUtf8(text, true); InvalidateLayout(); }
const std::string& Button::GetText() const noexcept { return m_Text; }
void Button::SetAccent(bool accent) { m_Accent = accent; InvalidatePaint(); }
Size Button::OnMeasure(Size) { return {std::max(72.0f, MeasureText(m_Text).m_Width + 28), GetTheme().m_ControlHeight}; }
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
	if ((event.m_Key == Key::Space || event.m_Key == Key::Enter) && !event.m_Repeat)
	{
		event.m_Handled = true;
		Activate();
	}
}
void Button::OnFocusChanged(bool focused)
{
	if (!focused) { m_Pressed = m_Armed = false; ReleasePointer(); }
	InvalidatePaint();
}
void Button::Activate() { m_Clicked.Emit(); }
void Button::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const auto size = GetActualSize();
	const Color resting = GetStyle().m_Background.value_or(m_Accent && IsEnabled() ? theme.m_Accent : theme.m_Surface);
	const Color hover = GetStyle().m_Hover.value_or(m_Accent ? theme.m_AccentHover : theme.m_Hover);
	const auto background = InteractiveColor(resting, hover, GetStyle().m_Pressed.value_or(m_Accent ? theme.m_AccentHover : theme.m_Pressed), m_Pressed && m_Armed);
	canvas.FillRectangle(GetLocalBounds(), background, GetRadius());
	canvas.StrokeRectangle(GetLocalBounds(), GetStyle().m_Border.value_or(m_Accent ? background : theme.m_Border), 1, GetRadius());
	const auto measured = canvas.MeasureText(m_Text, GetFontSize());
	canvas.DrawText(m_Text, {(size.m_Width - measured.m_Width) * 0.5f, (size.m_Height - measured.m_Height) * 0.5f},
		m_Accent && IsEnabled() ? theme.m_OnAccent : GetForeground(), GetFontSize());
	PaintFocus(canvas);
}
}
