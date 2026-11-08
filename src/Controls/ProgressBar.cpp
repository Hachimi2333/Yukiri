#include <Yukiri/Controls/ProgressBar.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <cmath>

namespace Yukiri
{
ProgressBar::ProgressBar(float value) { SetValue(value); }
void ProgressBar::SetValue(float value)
{
	if (!std::isfinite(value)) return;
	value = std::clamp(value, 0.0f, 1.0f);
	if (m_Value == value) return;
	m_Value = value;
	m_ValueVisual.Retarget(value, MotionDuration(GetTheme().m_Motion.m_SelectionDuration));
	RequestAnimationFrame();
	InvalidatePaint();
}
bool ProgressBar::OnAnimate(std::chrono::nanoseconds delta)
{
	const float previous = m_ValueVisual.GetValue();
	m_ValueVisual.Retarget(m_Value, MotionDuration(GetTheme().m_Motion.m_SelectionDuration));
	const bool active = m_ValueVisual.Advance(delta);
	if (previous != m_ValueVisual.GetValue()) InvalidatePaint();
	return active;
}
float ProgressBar::GetValue() const noexcept { return m_Value; }
Size ProgressBar::OnMeasure(Size) { return {200, 6}; }
void ProgressBar::OnPaint(Canvas& canvas)
{
	const auto size = GetActualSize();
	canvas.FillRectangle(GetLocalBounds(), GetTheme().m_Border, size.m_Height * 0.5f);
	canvas.FillRectangle({0, 0, size.m_Width * m_ValueVisual.GetValue(), size.m_Height}, GetStyle().m_Background.value_or(GetTheme().m_Accent), size.m_Height * 0.5f);
}
}
