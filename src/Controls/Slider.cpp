#include <Yukiri/Controls/Slider.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <cmath>

namespace Yukiri
{
Slider::Slider(float value) { SetFocusable(true); SetCursor(Cursor::Hand); SetValue(value); }
Result<void> Slider::SetRange(float minimum, float maximum)
{
	if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum >= maximum)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "Slider minimum must be smaller than maximum."});
	m_Minimum = minimum;
	m_Maximum = maximum;
	m_ValueVisual.Snap(std::clamp(m_Value, m_Minimum, m_Maximum));
	InvalidatePaint();
	SetValue(m_Value);
	return {};
}
void Slider::SetValue(float value)
{
	if (!std::isfinite(value)) return;
	value = std::clamp(value, m_Minimum, m_Maximum);
	if (m_Dragging) { m_ValueVisual.Snap(value); InvalidatePaint(); }
	if (m_Value == value) return;
	m_Value = value;
	m_ValueVisual.Retarget(value, m_Dragging ? std::chrono::nanoseconds::zero() : MotionDuration(GetTheme().m_Motion.m_SelectionDuration));
	RequestAnimationFrame();
	InvalidatePaint();
	m_ValueChanged.Emit(value);
}
bool Slider::OnAnimate(std::chrono::nanoseconds delta)
{
	const float previous = m_ValueVisual.GetValue();
	m_ValueVisual.Retarget(m_Value, m_Dragging ? std::chrono::nanoseconds::zero() : MotionDuration(GetTheme().m_Motion.m_SelectionDuration));
	const bool active = m_ValueVisual.Advance(delta);
	if (previous != m_ValueVisual.GetValue()) InvalidatePaint();
	return active;
}
float Slider::GetValue() const noexcept { return m_Value; }
Size Slider::OnMeasure(Size) { return {200, GetTheme().m_ControlHeight}; }
void Slider::SetFromPoint(Point point)
{
	const float fraction = std::clamp((ToLocal(point).m_X - 8) / std::max(1.0f, GetActualSize().m_Width - 16), 0.0f, 1.0f);
	SetValue(m_Minimum + fraction * (m_Maximum - m_Minimum));
}
void Slider::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	m_Dragging = true;
	CapturePointer();
	event.m_Handled = true;
	SetFromPoint(event.m_Position);
}
void Slider::OnPointerMove(PointerEvent& event) { if (m_Dragging) { event.m_Handled = true; SetFromPoint(event.m_Position); } }
void Slider::OnPointerUp(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left || !m_Dragging) return;
	m_Dragging = false;
	ReleasePointer();
	event.m_Handled = true;
}
void Slider::OnFocusChanged(bool focused) { if (!focused) { m_Dragging = false; ReleasePointer(); } InvalidatePaint(); }
void Slider::OnKeyDown(KeyEvent& event)
{
	float value = m_Value;
	const float step = (m_Maximum - m_Minimum) * (event.m_Modifiers.m_Shift ? 0.1f : 0.01f);
	if (event.m_Key == Key::Left || event.m_Key == Key::Down) value -= step;
	else if (event.m_Key == Key::Right || event.m_Key == Key::Up) value += step;
	else if (event.m_Key == Key::Home) value = m_Minimum;
	else if (event.m_Key == Key::End) value = m_Maximum;
	else return;
	event.m_Handled = true;
	SetValue(value);
}
void Slider::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const float width = std::max(0.0f, GetActualSize().m_Width - 16);
	const float y = GetActualSize().m_Height * 0.5f;
	const float x = 8 + width * std::clamp((m_ValueVisual.GetValue() - m_Minimum) / (m_Maximum - m_Minimum), 0.0f, 1.0f);
	canvas.FillRectangle({8, y - 2, width, 4}, theme.m_Border, 2);
	canvas.FillRectangle({8, y - 2, std::max(0.0f, x - 8), 4}, IsEnabled() ? theme.m_Accent : theme.m_MutedText, 2);
	canvas.FillRectangle({x - 7, y - 7, 14, 14}, InteractiveColor(theme.m_Surface, theme.m_Hover, theme.m_Pressed, m_Dragging), 7);
	canvas.StrokeRectangle({x - 7, y - 7, 14, 14}, IsEnabled() ? theme.m_Accent : theme.m_MutedText, 2, 7);
	PaintFocus(canvas);
}
}
