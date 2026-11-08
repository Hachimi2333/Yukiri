#include <Yukiri/Controls/ToggleSwitch.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Visuals.hpp"

namespace Yukiri
{
Size ToggleSwitch::OnMeasure(Size) { return {MeasureText(GetText()).m_Width + 54, GetTheme().m_ControlHeight}; }
void ToggleSwitch::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const float y = (GetActualSize().m_Height - 22) * 0.5f;
	const float amount = m_CheckVisual.GetValue();
	canvas.FillRectangle({1, y, 38, 22}, Detail::Mix(InteractiveColor(theme.m_Border, theme.m_Hover, theme.m_Pressed, m_Pressed),
		IsEnabled() ? theme.m_Accent : theme.m_MutedText, amount), 11);
	canvas.FillRectangle({4 + 15 * amount, y + 3, 16, 16}, Color::FromHex(0xFFFFFF), 8);
	const float height = canvas.MeasureText(GetText(), GetFontSize()).m_Height;
	canvas.DrawText(GetText(), {49, (GetActualSize().m_Height - height) * 0.5f}, GetForeground(), GetFontSize());
	PaintFocus(canvas);
}
}
