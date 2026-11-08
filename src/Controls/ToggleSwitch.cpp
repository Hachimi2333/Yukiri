#include <Yukiri/Controls/ToggleSwitch.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Visuals.hpp"

namespace Yukiri
{
Size ToggleSwitch::OnMeasure(Size)
{
	m_LabelSize = MeasureText(GetText());
	return {m_LabelSize.m_Width + 60, std::max(GetTheme().m_ControlHeight, m_LabelSize.m_Height + 12)};
}
Rect ToggleSwitch::GetFocusBounds() const noexcept { return Detail::SelectionFocusBounds(GetActualSize(), m_LabelSize, 54, 38, 22); }
void ToggleSwitch::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const float y = (GetActualSize().m_Height - 22) * 0.5f;
	const float amount = m_CheckVisual.GetValue();
	canvas.FillRectangle({6, y, 38, 22}, Detail::Mix(InteractiveColor(theme.m_Border, theme.m_Hover, theme.m_Pressed, m_Pressed),
		IsEnabled() ? theme.m_Accent : theme.m_MutedText, amount), 11);
	canvas.FillRectangle({9 + 15 * amount, y + 3, 16, 16}, Color::FromHex(0xFFFFFF), 8);
	const float height = canvas.MeasureText(GetText(), GetFontSize()).m_Height;
	canvas.PushClip({54, 4, std::max(0.0f, GetActualSize().m_Width - 60), std::max(0.0f, GetActualSize().m_Height - 8)});
	canvas.DrawText(GetText(), {54, (GetActualSize().m_Height - height) * 0.5f}, GetForeground(), GetFontSize());
	canvas.PopClip();
	PaintFocus(canvas);
}
}
