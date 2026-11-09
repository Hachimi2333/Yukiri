#pragma once

#include <string>

namespace Yukiri
{
struct Font
{
	std::string m_Family;
	int m_Weight = 400;
	bool m_Italic = false;
	bool operator==(const Font&) const = default;
};
}
