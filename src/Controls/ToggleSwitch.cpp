#include <Yukiri/Controls/ToggleSwitch.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri
{
Size ToggleSwitch::OnMeasure(Size) { return {MeasureText(GetText()).m_Width + 54, GetTheme().m_ControlHeight}; }
void ToggleSwitch::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const float y = (GetActualSize().m_Height - 22) * 0.5f;
	canvas.FillRectangle({1, y, 38, 22}, IsChecked() && IsEnabled() ? theme.m_Accent :
		InteractiveColor(theme.m_Border, theme.m_Hover, theme.m_Pressed, m_Pressed), 11);
	canvas.FillRectangle({IsChecked() ? 19.0f : 4.0f, y + 3, 16, 16}, Color::FromHex(0xFFFFFF), 8);
	const float height = canvas.MeasureText(GetText(), GetFontSize()).m_Height;
	canvas.DrawText(GetText(), {49, (GetActualSize().m_Height - height) * 0.5f}, GetForeground(), GetFontSize());
	PaintFocus(canvas);
}
}
