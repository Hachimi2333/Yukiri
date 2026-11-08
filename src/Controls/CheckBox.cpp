#include <Yukiri/Controls/CheckBox.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Visuals.hpp"

namespace Yukiri
{
CheckBox::CheckBox(std::string text, bool checked) : Button(std::move(text)), m_CheckVisual(checked ? 1.0f : 0.0f), m_Checked(checked) {}
bool CheckBox::IsChecked() const noexcept { return m_Checked; }
void CheckBox::SetChecked(bool checked)
{
	if (m_Checked == checked) return;
	m_Checked = checked;
	m_CheckVisual.Retarget(checked ? 1.0f : 0.0f, MotionDuration(GetTheme().m_Motion.m_SelectionDuration));
	RequestAnimationFrame();
	InvalidatePaint();
	m_CheckedChanged.Emit(checked);
}
bool CheckBox::OnAnimate(std::chrono::nanoseconds delta)
{
	const float previous = m_CheckVisual.GetValue();
	m_CheckVisual.Retarget(m_Checked ? 1.0f : 0.0f, MotionDuration(GetTheme().m_Motion.m_SelectionDuration));
	const bool active = m_CheckVisual.Advance(delta);
	if (previous != m_CheckVisual.GetValue()) InvalidatePaint();
	return active;
}
void CheckBox::Activate()
{
	const auto lifetime = ObserveLifetime();
	SetChecked(!m_Checked);
	if (!lifetime.expired()) Button::Activate();
}
Size CheckBox::OnMeasure(Size) { return {MeasureText(GetText()).m_Width + 32, GetTheme().m_ControlHeight}; }
void CheckBox::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const float y = (GetActualSize().m_Height - 18) * 0.5f;
	const float amount = m_CheckVisual.GetValue();
	const auto accent = IsEnabled() ? theme.m_Accent : theme.m_MutedText;
	canvas.FillRectangle({1, y, 18, 18}, Detail::Mix(InteractiveColor(theme.m_Surface, theme.m_Hover, theme.m_Pressed, m_Pressed), accent, amount), 4);
	canvas.StrokeRectangle({1, y, 18, 18}, Detail::Mix(theme.m_Border, accent, amount), 1, 4);
	if (amount > 0)
	{
		canvas.DrawLine({5, y + 9}, {9, y + 13}, theme.m_OnAccent.WithAlpha(amount), 1.8f);
		canvas.DrawLine({9, y + 13}, {15, y + 5}, theme.m_OnAccent.WithAlpha(amount), 1.8f);
	}
	const float height = canvas.MeasureText(GetText(), GetFontSize()).m_Height;
	canvas.DrawText(GetText(), {29, (GetActualSize().m_Height - height) * 0.5f}, GetForeground(), GetFontSize());
	PaintFocus(canvas);
}
}
