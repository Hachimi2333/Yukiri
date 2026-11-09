#pragma once

#include "TextComposition.hpp"

namespace Yukiri::Detail
{
struct TextVisual
{
	TextDisplay m_Display;
	std::vector<std::pair<std::size_t, std::size_t>> m_Positions;
	std::size_t ToVisual(std::size_t offset) const
	{
		if (m_Positions.empty()) return std::min(offset, m_Display.m_Text.size());
		auto after = std::ranges::upper_bound(m_Positions, offset, {}, &std::pair<std::size_t, std::size_t>::first);
		return after == m_Positions.begin() ? 0 : std::prev(after)->second;
	}
	std::size_t ToSource(std::size_t offset) const
	{
		if (m_Positions.empty()) return std::min(offset, m_Display.m_Text.size());
		auto after = std::ranges::lower_bound(m_Positions, offset, {}, &std::pair<std::size_t, std::size_t>::second);
		if (after == m_Positions.begin()) return 0;
		if (after == m_Positions.end()) return m_Positions.back().first;
		const auto previous = std::prev(after);
		return offset - previous->second < after->second - offset ? previous->first : after->first;
	}
};
inline TextVisual BuildTextVisual(TextDisplay display, bool password)
{
	TextVisual result;
	if (!password && display.m_Text.find('\t') == std::string::npos) { result.m_Display = std::move(display); return result; }
	std::string visible;
	const auto boundaries = GraphemeBoundaries(display.m_Text);
	std::size_t column = 0;
	for (std::size_t index = 0; index + 1 < boundaries.size(); ++index)
	{
		result.m_Positions.emplace_back(boundaries[index], visible.size());
		const auto value = std::string_view(display.m_Text).substr(boundaries[index], boundaries[index + 1] - boundaries[index]);
		if (password) visible += "\xE2\x80\xA2";
		else if (value == "\t") { const auto count = 4 - column % 4; visible.append(count, ' '); column += count; }
		else { visible += value; if (value == "\n") column = 0; else ++column; }
	}
	result.m_Positions.emplace_back(display.m_Text.size(), visible.size());
	display.m_Caret = result.ToVisual(display.m_Caret);
	display.m_SelectionStart = result.ToVisual(display.m_SelectionStart);
	display.m_SelectionEnd = result.ToVisual(display.m_SelectionEnd);
	display.m_CompositionStart = result.ToVisual(display.m_CompositionStart);
	display.m_CompositionEnd = result.ToVisual(display.m_CompositionEnd);
	display.m_Text = std::move(visible);
	result.m_Display = std::move(display);
	return result;
}
}
