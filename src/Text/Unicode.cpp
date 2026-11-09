#include "Unicode.hpp"
#include <graphemebreak.h>
#include <wordbreak.h>
#include <linebreak.h>
#include <algorithm>

namespace Yukiri::Detail
{
std::vector<Codepoint> DecodeUtf8(std::string_view text)
{
	std::vector<Codepoint> result;
	for (std::size_t offset = 0; offset < text.size();)
	{
		const auto first = static_cast<unsigned char>(text[offset]);
		std::uint32_t value = first;
		std::size_t count = 1;
		if (first >= 0xC2 && first <= 0xDF) { value = first & 0x1F; count = 2; }
		else if (first >= 0xE0 && first <= 0xEF) { value = first & 0x0F; count = 3; }
		else if (first >= 0xF0 && first <= 0xF4) { value = first & 7; count = 4; }
		else if (first >= 0x80) value = 0xFFFD;
		bool valid = offset + count <= text.size();
		for (std::size_t i = 1; valid && i < count; ++i)
		{
			const auto continuation = static_cast<unsigned char>(text[offset + i]);
			if ((continuation & 0xC0) != 0x80) valid = false;
			else value = (value << 6) | (continuation & 0x3F);
		}
		if (!valid || (count == 2 && value < 0x80) || (count == 3 && value < 0x800) ||
			(count == 4 && value < 0x10000) || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
		{
			value = 0xFFFD;
			count = 1;
		}
		result.push_back({value, offset, offset + count});
		offset += count;
	}
	return result;
}

std::string SanitizeUtf8(std::string_view text, bool singleLine)
{
	std::string result;
	for (const auto codepoint : DecodeUtf8(text))
	{
		if (singleLine && (codepoint.m_Value == '\r' || codepoint.m_Value == '\n' || codepoint.m_Value == '\t'))
		{
			if (codepoint.m_Value != '\r') result += ' ';
			continue;
		}
		if (codepoint.m_Value == 0xFFFD || codepoint.m_Value == 0) result += "\xEF\xBF\xBD";
		else result.append(text.substr(codepoint.m_Start, codepoint.m_End - codepoint.m_Start));
	}
	return result;
}

std::string NormalizeText(std::string_view text, bool singleLine)
{
	if (singleLine) return SanitizeUtf8(text, true);
	const auto sanitized = SanitizeUtf8(text);
	std::string result;
	result.reserve(sanitized.size());
	for (std::size_t index = 0; index < sanitized.size(); ++index)
	{
		if (sanitized[index] != '\r') result += sanitized[index];
		else
		{
			result += '\n';
			if (index + 1 < sanitized.size() && sanitized[index + 1] == '\n') ++index;
		}
	}
	return result;
}
namespace
{
std::vector<std::size_t> Boundaries(std::string_view text, int type)
{
	std::vector<std::size_t> result{0};
	if (text.empty()) return result;
	std::vector<char> breaks(text.size());
	const auto* bytes = reinterpret_cast<const utf8_t*>(text.data());
	if (type == 0) set_graphemebreaks_utf8(bytes, text.size(), nullptr, breaks.data());
	else if (type == 1) set_wordbreaks_utf8(bytes, text.size(), nullptr, breaks.data());
	else set_linebreaks_utf8(bytes, text.size(), nullptr, breaks.data());
	for (std::size_t i = 0; i < text.size(); ++i)
		if ((type < 2 && breaks[i] == 0) || (type == 2 && (breaks[i] == LINEBREAK_ALLOWBREAK || breaks[i] == LINEBREAK_MUSTBREAK)))
			result.push_back(i + 1);
	if (result.back() != text.size()) result.push_back(text.size());
	return result;
}
}
std::vector<std::size_t> GraphemeBoundaries(std::string_view text) { return Boundaries(text, 0); }
std::vector<std::size_t> WordBoundaries(std::string_view text) { return Boundaries(text, 1); }
std::vector<std::size_t> LineBoundaries(std::string_view text) { return Boundaries(text, 2); }
std::size_t PreviousBoundary(const std::vector<std::size_t>& boundaries, std::size_t offset)
{
	auto found = std::ranges::lower_bound(boundaries, offset);
	return found == boundaries.begin() ? 0 : *std::prev(found);
}
std::size_t NextBoundary(const std::vector<std::size_t>& boundaries, std::size_t offset)
{
	auto found = std::ranges::upper_bound(boundaries, offset);
	return found == boundaries.end() ? boundaries.back() : *found;
}
}
