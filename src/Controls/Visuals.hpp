#pragma once

#include <Yukiri/Core/Geometry.hpp>

namespace Yukiri::Detail
{
inline Color Mix(Color from, Color to, float amount)
{
	return {from.m_R + (to.m_R - from.m_R) * amount, from.m_G + (to.m_G - from.m_G) * amount,
		from.m_B + (to.m_B - from.m_B) * amount, from.m_A + (to.m_A - from.m_A) * amount};
}
}
