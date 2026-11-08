#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri
{
RadioButton::RadioButton(std::string text, std::string group) : CheckBox(std::move(text)), m_Group(std::move(group)) {}
void RadioButton::SetChecked(bool checked)
{
	if (checked && GetParent())
	{
		const auto lifetime = ObserveLifetime();
		std::vector<std::pair<RadioButton*, std::weak_ptr<int>>> siblings;
		for (const auto& child : GetParent()->GetChildren())
			if (auto* radio = dynamic_cast<RadioButton*>(child.get()); radio && radio != this && radio->m_Group == m_Group)
				siblings.emplace_back(radio, radio->ObserveLifetime());
		for (auto& [radio, token] : siblings)
		{
			if (!token.expired()) radio->CheckBox::SetChecked(false);
			if (lifetime.expired()) return;
		}
	}
	CheckBox::SetChecked(checked);
}
void RadioButton::Activate()
{
	const auto lifetime = ObserveLifetime();
	SetChecked(true);
	if (!lifetime.expired()) Button::Activate();
}
void RadioButton::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const float y = (GetActualSize().m_Height - 18) * 0.5f;
	canvas.FillRectangle({1, y, 18, 18}, InteractiveColor(theme.m_Surface, theme.m_Hover, theme.m_Pressed, m_Pressed), 9);
	canvas.StrokeRectangle({1, y, 18, 18}, IsChecked() ? theme.m_Accent : theme.m_Border, IsChecked() ? 2.0f : 1.0f, 9);
	if (IsChecked()) canvas.FillRectangle({6, y + 5, 8, 8}, IsEnabled() ? theme.m_Accent : theme.m_MutedText, 4);
	const float height = canvas.MeasureText(GetText(), GetFontSize()).m_Height;
	canvas.DrawText(GetText(), {29, (GetActualSize().m_Height - height) * 0.5f}, GetForeground(), GetFontSize());
	PaintFocus(canvas);
}
}
