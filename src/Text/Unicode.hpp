#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Yukiri::Detail
{
struct Codepoint
{
	std::uint32_t m_Value;
	std::size_t m_Start;
	std::size_t m_End;
};
std::vector<Codepoint> DecodeUtf8(std::string_view text);
std::string SanitizeUtf8(std::string_view text, bool singleLine = false);
std::string NormalizeText(std::string_view text, bool singleLine);
std::vector<std::size_t> GraphemeBoundaries(std::string_view text);
std::vector<std::size_t> WordBoundaries(std::string_view text);
std::vector<std::size_t> LineBoundaries(std::string_view text);
std::size_t PreviousBoundary(const std::vector<std::size_t>& boundaries, std::size_t offset);
std::size_t NextBoundary(const std::vector<std::size_t>& boundaries, std::size_t offset);
}
