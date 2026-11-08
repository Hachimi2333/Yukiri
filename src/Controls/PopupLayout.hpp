#pragma once

#include <Yukiri/Core/Geometry.hpp>

namespace Yukiri::Detail
{
struct PopupPlacement
{
	Rect m_Bounds;
	int m_VisibleRows = 0;
};
inline PopupPlacement PlacePopup(Rect anchor, Rect viewport, int items, float rowHeight)
{
	if (viewport.Empty() || items <= 0 || rowHeight <= 0) return {};
	const int capacity = std::max(1, static_cast<int>((viewport.m_Height - 16) / rowHeight));
	const int rows = std::min({7, items, capacity});
	const float height = std::min(viewport.m_Height, static_cast<float>(rows) * rowHeight + 8);
	const float width = std::min(std::max(160.0f, anchor.m_Width), viewport.m_Width);
	const float preferredY = anchor.Bottom() + height + 4 <= viewport.Bottom() ? anchor.Bottom() + 4 : anchor.m_Y - height - 4;
	return {{std::clamp(anchor.m_X, viewport.m_X, viewport.Right() - width),
		std::clamp(preferredY, viewport.m_Y, viewport.Bottom() - height), width, height}, rows};
}
}
