#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Visuals.hpp"

namespace Yukiri
{
RadioButton::RadioButton(std::string text, std::string group) : CheckBox(std::move(text)), m_Group(std::move(group)) {}
std::string_view RadioButton::GetGroup() const noexcept { return m_Group; }
void RadioButton::OnArrange(Rect) { if (IsChecked()) SetChecked(true); }
bool RadioButton::IsTabStop() const noexcept
{
	if (!Widget::IsTabStop()) return false;
	const RadioButton* first = nullptr;
	bool selected = false;
	if (GetParent()) for (const auto& child : GetParent()->GetChildren())
		if (auto* radio = dynamic_cast<RadioButton*>(child.get()); radio && radio->m_Group == m_Group && radio->IsVisible() && radio->IsEnabled() && radio->Widget::IsTabStop())
		{
			if (!first) first = radio;
			if (radio->IsChecked()) selected = true;
		}
	return IsChecked() || (!selected && (!first || first == this));
}
void RadioButton::OnKeyDown(KeyEvent& event)
{
	if (event.m_Modifiers.m_Alt || event.m_Modifiers.m_Control || event.m_Modifiers.m_Super) { CheckBox::OnKeyDown(event); return; }
	const bool previous = event.m_Key == Key::Left || event.m_Key == Key::Up;
	if ((!previous && event.m_Key != Key::Right && event.m_Key != Key::Down) || !GetParent()) { CheckBox::OnKeyDown(event); return; }
	Dispatch([&]
	{
		std::vector<RadioButton*> group;
		for (const auto& child : GetParent()->GetChildren())
			if (auto* radio = dynamic_cast<RadioButton*>(child.get()); radio && radio->m_Group == m_Group && radio->IsEnabled() && radio->IsVisible()) group.push_back(radio);
		const auto found = std::ranges::find(group, this);
		if (found == group.end()) return;
		const auto index = static_cast<std::size_t>((static_cast<int>(found - group.begin()) + (previous ? -1 : 1) + static_cast<int>(group.size())) % static_cast<int>(group.size()));
		auto* target = group[index];
		target->RequestFocus();
		target->Activate();
		event.m_Handled = true;
	});
}
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
	canvas.FillRectangle({6, y, 18, 18}, InteractiveColor(theme.m_Surface, theme.m_Hover, theme.m_Pressed, m_Pressed), 9);
	const float amount = m_CheckVisual.GetValue();
	const auto accent = IsEnabled() ? theme.m_Accent : theme.m_MutedText;
	canvas.StrokeRectangle({6, y, 18, 18}, Detail::Mix(theme.m_Border, accent, amount), 1 + amount, 9);
	const float radius = 4 * amount;
	canvas.FillRectangle({15 - radius, y + 9 - radius, radius * 2, radius * 2}, accent.WithAlpha(amount), radius);
	const float height = canvas.MeasureText(GetText(), GetFontSize()).m_Height;
	canvas.PushClip({34, 4, std::max(0.0f, GetActualSize().m_Width - 40), std::max(0.0f, GetActualSize().m_Height - 8)});
	canvas.DrawText(GetText(), {34, (GetActualSize().m_Height - height) * 0.5f}, GetForeground(), GetFontSize());
	canvas.PopClip();
	PaintFocus(canvas);
}
}
