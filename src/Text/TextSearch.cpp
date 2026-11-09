#include "TextEditor.hpp"
#include <span>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Yukiri::Detail
{
namespace
{
struct Unit { std::uint32_t m_Value; std::size_t m_Start; std::size_t m_End; };
std::vector<Unit> SearchUnits(std::string_view text, bool matchCase)
{
	std::vector<Unit> result;
	for (const auto point : DecodeUtf8(text))
	{
#if defined(_WIN32)
		if (!matchCase)
		{
			wchar_t source[2]{};
			int length = 1;
			if (point.m_Value <= 0xFFFF) source[0] = static_cast<wchar_t>(point.m_Value);
			else { source[0] = static_cast<wchar_t>(0xD800 + ((point.m_Value - 0x10000) >> 10)); source[1] = static_cast<wchar_t>(0xDC00 + ((point.m_Value - 0x10000) & 1023)); length = 2; }
			wchar_t mapped[8]{};
			const int count = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, source, length, mapped, 8, nullptr, nullptr, 0);
			if (count)
			{
				for (int index = 0; index < count; ++index)
				{
					std::uint32_t value = mapped[index];
					if (value >= 0xD800 && value <= 0xDBFF && index + 1 < count && mapped[index + 1] >= 0xDC00 && mapped[index + 1] <= 0xDFFF)
						value = 0x10000 + ((value - 0xD800) << 10) + (mapped[++index] - 0xDC00);
					result.push_back({value, point.m_Start, point.m_End});
				}
				continue;
			}
		}
#else
		if (!matchCase && point.m_Value >= 'a' && point.m_Value <= 'z') { result.push_back({point.m_Value - 'a' + 'A', point.m_Start, point.m_End}); continue; }
#endif
		result.push_back({point.m_Value, point.m_Start, point.m_End});
	}
	return result;
}
std::vector<std::pair<std::size_t, std::size_t>> Matches(std::string_view text, std::string_view pattern, bool matchCase)
{
	std::vector<std::pair<std::size_t, std::size_t>> result;
	if (pattern.empty()) return result;
	const auto haystack = SearchUnits(text, matchCase), needle = SearchUnits(pattern, matchCase);
	if (needle.empty()) return result;
	std::vector<std::size_t> prefix(needle.size());
	for (std::size_t index = 1, matched = 0; index < needle.size(); ++index)
	{
		while (matched && needle[index].m_Value != needle[matched].m_Value) matched = prefix[matched - 1];
		if (needle[index].m_Value == needle[matched].m_Value) ++matched;
		prefix[index] = matched;
	}
	const auto boundaries = GraphemeBoundaries(text);
	for (std::size_t index = 0, matched = 0; index < haystack.size(); ++index)
	{
		while (matched && haystack[index].m_Value != needle[matched].m_Value) matched = prefix[matched - 1];
		if (haystack[index].m_Value == needle[matched].m_Value) ++matched;
		if (matched != needle.size()) continue;
		const auto first = index + 1 - matched;
		const auto start = haystack[first].m_Start, end = haystack[index].m_End;
		const bool complete = (first == 0 || haystack[first - 1].m_Start != start) && (index + 1 == haystack.size() || haystack[index + 1].m_End != end);
		if (complete && std::ranges::binary_search(boundaries, start) && std::ranges::binary_search(boundaries, end)) result.emplace_back(start, end);
		matched = prefix[matched - 1];
	}
	return result;
}
}
std::optional<std::pair<std::size_t, std::size_t>> TextEditor::Find(std::string_view pattern, std::size_t from, bool matchCase, bool backwards) const
{
	const auto matches = Matches(m_Text, NormalizeText(pattern, m_SingleLine), matchCase);
	if (backwards)
	{
		for (auto match = matches.rbegin(); match != matches.rend(); ++match) if (match->second <= from) return *match;
	}
	else for (const auto& match : matches) if (match.first >= from) return match;
	return {};
}
std::size_t TextEditor::ReplaceAll(std::string_view pattern, std::string_view replacement, bool matchCase)
{
	const auto matches = Matches(m_Text, NormalizeText(pattern, m_SingleLine), matchCase);
	if (matches.empty()) return 0;
	const auto value = NormalizeText(replacement, m_SingleLine);
	std::string result;
	std::size_t position = 0, count = 0;
	for (const auto& [start, end] : matches)
	{
		if (start < position) continue;
		result.append(m_Text, position, start - position);
		result += value;
		position = end;
		++count;
	}
	result.append(m_Text, position, m_Text.size() - position);
	Apply(0, m_Text.size(), std::move(result), EditKind::Other);
	return count;
}
}
