#include "ResponsivePanel.hpp"
#include <algorithm>
#include <cmath>

namespace YukiriDemo
{
ResponsivePanel::ResponsivePanel(float columnWidth, int maxColumns, float spacing)
	: m_ColumnWidth(columnWidth), m_MaxColumns(maxColumns), m_Spacing(spacing) {}
int ResponsivePanel::Columns(float width) const
{
	if (!std::isfinite(width)) return m_MaxColumns;
	return std::clamp(static_cast<int>((width + m_Spacing) / (m_ColumnWidth + m_Spacing)), 1, m_MaxColumns);
}
Yukiri::Size ResponsivePanel::OnMeasure(Yukiri::Size available)
{
	const int columns = Columns(available.m_Width);
	const float width = std::isfinite(available.m_Width) ?
		std::max(0.0f, (available.m_Width - static_cast<float>(columns - 1) * m_Spacing) / static_cast<float>(columns)) : m_ColumnWidth;
	float total = 0;
	float rowHeight = 0;
	int index = 0;
	for (const auto& child : GetChildren())
	{
		child->Measure({width, available.m_Height});
		rowHeight = std::max(rowHeight, child->GetDesiredSize().m_Height);
		if (++index % columns == 0) { total += rowHeight + m_Spacing; rowHeight = 0; }
	}
	if (index % columns) total += rowHeight + m_Spacing;
	return {width * static_cast<float>(columns) + static_cast<float>(columns - 1) * m_Spacing,
		std::max(0.0f, total - m_Spacing)};
}
void ResponsivePanel::OnArrange(Yukiri::Rect content)
{
	const auto& children = GetChildren();
	const int columns = Columns(content.m_Width);
	const float width = std::max(0.0f, (content.m_Width - static_cast<float>(columns - 1) * m_Spacing) / static_cast<float>(columns));
	float y = content.m_Y;
	for (std::size_t first = 0; first < children.size(); first += static_cast<std::size_t>(columns))
	{
		const auto end = std::min(children.size(), first + static_cast<std::size_t>(columns));
		float height = 0;
		for (std::size_t i = first; i < end; ++i)
		{
			children[i]->Measure({width, std::numeric_limits<float>::infinity()});
			height = std::max(height, children[i]->GetDesiredSize().m_Height);
		}
		for (std::size_t i = first; i < end; ++i)
			children[i]->Arrange({content.m_X + static_cast<float>(i - first) * (width + m_Spacing), y, width, height});
		y += height + m_Spacing;
	}
}
}
