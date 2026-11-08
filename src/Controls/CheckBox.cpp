#include <Yukiri/Controls/CheckBox.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri
{
CheckBox::CheckBox(std::string text, bool checked) : Button(std::move(text)), m_Checked(checked) {}
bool CheckBox::IsChecked() const noexcept { return m_Checked; }
void CheckBox::SetChecked(bool checked)
{
	if (m_Checked == checked) return;
	m_Checked = checked;
	InvalidatePaint();
	m_CheckedChanged.Emit(checked);
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
	canvas.FillRectangle({1, y, 18, 18}, m_Checked && IsEnabled() ? theme.m_Accent : InteractiveColor(theme.m_Surface, theme.m_Hover, theme.m_Pressed, m_Pressed), 4);
	canvas.StrokeRectangle({1, y, 18, 18}, m_Checked ? theme.m_Accent : theme.m_Border, 1, 4);
	if (m_Checked)
	{
		canvas.DrawLine({5, y + 9}, {9, y + 13}, theme.m_OnAccent, 1.8f);
		canvas.DrawLine({9, y + 13}, {15, y + 5}, theme.m_OnAccent, 1.8f);
	}
	const float height = canvas.MeasureText(GetText(), GetFontSize()).m_Height;
	canvas.DrawText(GetText(), {29, (GetActualSize().m_Height - height) * 0.5f}, GetForeground(), GetFontSize());
	PaintFocus(canvas);
}
}
