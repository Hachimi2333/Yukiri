#pragma once

#include <Yukiri/Render/Image.hpp>
#include <string>
#include <vector>

namespace Yukiri::Detail
{
struct ImagePixels
{
	std::uint32_t m_Width = 0;
	std::uint32_t m_Height = 0;
	std::vector<std::uint8_t> m_Bytes;
};
struct ImageData
{
	std::uint64_t m_Id = 0;
	ImagePixels m_Pixels;
	std::string m_Svg;
};
Result<ImagePixels> RasterizeSvg(std::string_view source, std::uint32_t width = 0, std::uint32_t height = 0);
Result<void> ValidateImageSize(std::uint32_t width, std::uint32_t height);
}
