#pragma once

#include <filesystem>
#include <vector>
#include <optional>
#include <Yukiri/Text/Font.hpp>

namespace Yukiri::Detail
{
struct FontSource
{
	std::filesystem::path m_Path;
	long m_Index = 0;
	std::string m_Family{};
};
std::vector<FontSource> DiscoverFonts();
std::optional<FontSource> ResolveFont(const Yukiri::Font& font);
}
