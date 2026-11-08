#pragma once

#include <filesystem>
#include <vector>

namespace Yukiri::Detail
{
struct FontSource
{
	std::filesystem::path m_Path;
	long m_Index = 0;
};
std::vector<FontSource> DiscoverFonts();
}
