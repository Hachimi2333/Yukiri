#pragma once

#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri::Detail
{
struct CanvasFactory
{
	static Canvas Create(DrawList& list, Size size, TextEngine* text = nullptr, float scale = 1) { return Canvas(list, size, text, scale); }
};
}
