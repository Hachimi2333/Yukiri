#include <Yukiri/Controls/ProgressBar.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <cmath>

namespace Yukiri
{
ProgressBar::ProgressBar(float value) { SetValue(value); }
void ProgressBar::SetIndeterminate(bool indeterminate)
{
	if (m_Indeterminate == indeterminate) return;
	m_Indeterminate = indeterminate;
	m_Phase = 0;
	InvalidatePaint();
	if (indeterminate && MotionDuration(std::chrono::milliseconds(1200)).count()) RequestAnimationFrame();
}
bool ProgressBar::IsIndeterminate() const noexcept { return m_Indeterminate; }
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
	if (m_Indeterminate)
	{
		const auto duration = MotionDuration(std::chrono::milliseconds(1200));
		if (!duration.count()) return false;
		m_Phase = std::fmod(m_Phase + std::chrono::duration<float>(delta).count() / std::chrono::duration<float>(duration).count(), 1.0f);
		InvalidatePaint();
		return true;
	}
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
	if (m_Indeterminate)
	{
		const bool moving = MotionDuration(std::chrono::milliseconds(1200)).count() > 0;
		const float x = moving ? size.m_Width * (m_Phase * 1.3f - 0.3f) : size.m_Width * 0.35f;
		canvas.PushClip(GetLocalBounds());
		canvas.FillRectangle({x, 0, size.m_Width * 0.3f, size.m_Height}, GetStyle().m_Background.value_or(GetTheme().m_Accent), size.m_Height * 0.5f);
		canvas.PopClip();
		if (moving) RequestAnimationFrame();
	}
	else canvas.FillRectangle({0, 0, size.m_Width * m_ValueVisual.GetValue(), size.m_Height}, GetStyle().m_Background.value_or(GetTheme().m_Accent), size.m_Height * 0.5f);
}
}
