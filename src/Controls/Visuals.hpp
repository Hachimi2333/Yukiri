#pragma once

#include <Yukiri/Core/Geometry.hpp>

namespace Yukiri::Detail
{
inline Rect SelectionFocusBounds(Size size, Size label, float textLeft, float iconWidth, float iconHeight) noexcept
{
	const float height = std::max(iconHeight, label.m_Height);
	const float right = label.m_Width > 0 ? textLeft + label.m_Width : 6 + iconWidth;
	return Rect{2, (size.m_Height - height) * 0.5f - 4, right + 2, height + 8}.Intersect(
		{2, 2, std::max(0.0f, size.m_Width - 4), std::max(0.0f, size.m_Height - 4)});
}

inline Color Mix(Color from, Color to, float amount)
{
	return {from.m_R + (to.m_R - from.m_R) * amount, from.m_G + (to.m_G - from.m_G) * amount,
		from.m_B + (to.m_B - from.m_B) * amount, from.m_A + (to.m_A - from.m_A) * amount};
}
}
