#include <Yukiri/Layout/StackPanel.hpp>
#include <algorithm>
#include <limits>

namespace Yukiri
{
StackPanel::StackPanel(Orientation orientation, float spacing) : m_Orientation(orientation), m_Spacing(std::max(0.0f, spacing)) {}
void StackPanel::SetOrientation(Orientation orientation) { m_Orientation = orientation; InvalidateLayout(); }
void StackPanel::SetSpacing(float spacing) { m_Spacing = std::max(0.0f, spacing); InvalidateLayout(); }
Size StackPanel::OnMeasure(Size available)
{
	const bool vertical = m_Orientation == Orientation::Vertical;
	if (vertical) available.m_Height = std::numeric_limits<float>::infinity();
	else available.m_Width = std::numeric_limits<float>::infinity();
	Size desired;
	std::size_t count = 0;
	for (auto* child : Children())
	{
		if (!child->IsVisible()) continue;
		child->Measure(available);
		const auto size = child->GetDesiredSize();
		if (vertical) { desired.m_Height += size.m_Height; desired.m_Width = std::max(desired.m_Width, size.m_Width); }
		else { desired.m_Width += size.m_Width; desired.m_Height = std::max(desired.m_Height, size.m_Height); }
		++count;
	}
	const float gap = count ? static_cast<float>(count - 1) * m_Spacing : 0;
	if (vertical) desired.m_Height += gap; else desired.m_Width += gap;
	return desired;
}
void StackPanel::OnArrange(Rect content)
{
	const bool vertical = m_Orientation == Orientation::Vertical;
	float offset = 0;
	for (auto* child : Children())
	{
		if (!child->IsVisible()) continue;
		const auto desired = child->GetDesiredSize();
		child->Arrange(vertical ? Rect{content.m_X, content.m_Y + offset, content.m_Width, desired.m_Height} :
			Rect{content.m_X + offset, content.m_Y, desired.m_Width, content.m_Height});
		offset += (vertical ? desired.m_Height : desired.m_Width) + m_Spacing;
	}
}
}
