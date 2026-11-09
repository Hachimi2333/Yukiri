#include <Yukiri/Layout/WrapPanel.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Yukiri
{
WrapPanel::WrapPanel(Orientation orientation) : m_Orientation(orientation) {}
void WrapPanel::SetOrientation(Orientation orientation) { m_Orientation = orientation; InvalidateLayout(); }
void WrapPanel::SetSpacing(float horizontal, float vertical)
{
	m_HorizontalSpacing = std::isfinite(horizontal) ? std::max(0.0f, horizontal) : 0;
	m_VerticalSpacing = std::isfinite(vertical) ? std::max(0.0f, vertical) : 0;
	InvalidateLayout();
}
Size WrapPanel::OnMeasure(Size available) { return Layout(available, {}, false); }
void WrapPanel::OnArrange(Rect content) { Layout({content.m_Width, content.m_Height}, {content.m_X, content.m_Y}, true); }
Size WrapPanel::Layout(Size available, Point origin, bool arrange)
{
	const bool horizontal = m_Orientation == Orientation::Horizontal;
	const float limit = horizontal ? available.m_Width : available.m_Height;
	const float gap = horizontal ? m_HorizontalSpacing : m_VerticalSpacing;
	const float lineGap = horizontal ? m_VerticalSpacing : m_HorizontalSpacing;
	struct Line { std::vector<Widget*> m_Items; float m_Main = 0; float m_Cross = 0; };
	std::vector<Line> lines(1);
	for (auto* child : Children())
	{
		if (!child->IsVisible()) continue;
		child->Measure(horizontal ? Size{available.m_Width, std::numeric_limits<float>::infinity()} :
			Size{std::numeric_limits<float>::infinity(), available.m_Height});
		const auto size = child->GetDesiredSize();
		const float main = horizontal ? size.m_Width : size.m_Height;
		const float cross = horizontal ? size.m_Height : size.m_Width;
		if (!lines.back().m_Items.empty() && lines.back().m_Main + gap + main > limit) lines.emplace_back();
		auto& line = lines.back();
		if (!line.m_Items.empty()) line.m_Main += gap;
		line.m_Items.push_back(child);
		line.m_Main += main;
		line.m_Cross = std::max(line.m_Cross, cross);
	}
	float mainSize = 0;
	float crossOffset = 0;
	for (const auto& line : lines)
	{
		float mainOffset = 0;
		for (auto* child : line.m_Items)
		{
			const auto size = child->GetDesiredSize();
			const float main = horizontal ? size.m_Width : size.m_Height;
			if (arrange) child->Arrange(horizontal ? Rect{origin.m_X + mainOffset, origin.m_Y + crossOffset, main, line.m_Cross} :
				Rect{origin.m_X + crossOffset, origin.m_Y + mainOffset, line.m_Cross, main});
			mainOffset += main + gap;
		}
		mainSize = std::max(mainSize, line.m_Main);
		crossOffset += line.m_Cross + lineGap;
	}
	crossOffset = std::max(0.0f, crossOffset - lineGap);
	return horizontal ? Size{mainSize, crossOffset} : Size{crossOffset, mainSize};
}
}
