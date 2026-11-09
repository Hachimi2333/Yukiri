#pragma once

#include <Yukiri/Core/Geometry.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Yukiri::Detail
{
struct TextLine
{
	std::size_t m_Start = 0;
	std::size_t m_End = 0;
	float m_Width = 0;
	float m_Height = 0;
};
using TextMeasure = std::function<Size(std::string_view)>;
using TextCarets = std::function<std::vector<std::pair<std::size_t, float>>(std::string_view)>;
std::vector<TextLine> LayoutText(std::string_view text, float width, bool wrap, const TextMeasure& measure, const TextCarets& carets = {});
std::string FitText(std::string_view text, float width, const TextMeasure& measure);
}
