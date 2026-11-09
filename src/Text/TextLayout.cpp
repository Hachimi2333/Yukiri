#include "TextLayout.hpp"
#include "Unicode.hpp"
#include <algorithm>
#include <cmath>

namespace Yukiri::Detail
{
std::vector<TextLine> LayoutText(std::string_view text, float width, bool wrap, const TextMeasure& measure, const TextCarets& carets)
{
	std::vector<TextLine> lines;
	std::size_t paragraph = 0;
	for (;;)
	{
		const auto newline = text.find('\n', paragraph);
		const auto limit = newline == std::string_view::npos ? text.size() : newline;
		const auto value = text.substr(paragraph, limit - paragraph);
		if (!wrap || !std::isfinite(width) || value.empty())
		{
			const auto size = measure(value);
			lines.push_back({paragraph, limit, size.m_Width, size.m_Height});
		}
		else
		{
			const auto boundaries = GraphemeBoundaries(value);
			const auto breaks = LineBoundaries(value);
			auto positions = carets ? carets(value) : std::vector<std::pair<std::size_t, float>>{};
			if (positions.size() != boundaries.size())
			{
				positions = {{std::size_t(0), 0.0f}};
				float x = 0;
				for (std::size_t index = 1; index < boundaries.size(); ++index)
				{
					x += measure(value.substr(boundaries[index - 1], boundaries[index] - boundaries[index - 1])).m_Width;
					positions.emplace_back(boundaries[index], x);
				}
			}
			std::size_t first = 0;
			while (first + 1 < boundaries.size())
			{
				const float target = positions[first].second + std::max(0.0f, width);
				auto after = std::upper_bound(positions.begin() + static_cast<std::ptrdiff_t>(first), positions.end(), target,
					[](float x, const auto& caret) { return x < caret.second; });
				std::size_t last = std::max(first + 1, static_cast<std::size_t>(after - positions.begin()) - 1);
				last = std::min(last, boundaries.size() - 1);
				if (last + 1 < boundaries.size())
				{
					auto preferred = std::ranges::upper_bound(breaks, boundaries[last]);
					if (preferred != breaks.begin() && *std::prev(preferred) > boundaries[first])
						last = static_cast<std::size_t>(std::ranges::lower_bound(boundaries, *std::prev(preferred)) - boundaries.begin());
				}
				auto size = measure(value.substr(boundaries[first], boundaries[last] - boundaries[first]));
				while (last > first + 1 && size.m_Width > width)
				{
					--last;
					size = measure(value.substr(boundaries[first], boundaries[last] - boundaries[first]));
				}
				lines.push_back({paragraph + boundaries[first], paragraph + boundaries[last], size.m_Width, size.m_Height});
				first = last;
			}
		}
		if (newline == std::string_view::npos) break;
		paragraph = newline + 1;
	}
	return lines;
}
std::string FitText(std::string_view text, float width, const TextMeasure& measure)
{
	if (measure(text).m_Width <= width) return std::string(text);
	const std::string_view ellipsis = "\xE2\x80\xA6";
	if (measure(ellipsis).m_Width > width) return {};
	const auto boundaries = GraphemeBoundaries(text);
	std::size_t low = 0;
	std::size_t high = boundaries.size() - 1;
	while (low < high)
	{
		const auto middle = (low + high + 1) / 2;
		const auto candidate = std::string(text.substr(0, boundaries[middle])) + std::string(ellipsis);
		if (measure(candidate).m_Width <= width) low = middle; else high = middle - 1;
	}
	return std::string(text.substr(0, boundaries[low])) + std::string(ellipsis);
}
}
