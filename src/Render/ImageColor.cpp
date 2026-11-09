#include "ImageColor.hpp"
#include <array>
#include <cmath>

namespace Yukiri::Detail
{
void PrepareImageUpload(std::span<std::uint8_t> rgba)
{
	// Store sRGB-encoded, linear-premultiplied RGB so hardware filtering does
	// not introduce dark fringes at transparent image boundaries.
	static const auto table = []
	{
		std::array<std::array<std::uint8_t, 256>, 256> values{};
		for (std::size_t alpha = 0; alpha < 256; ++alpha)
			for (std::size_t channel = 0; channel < 256; ++channel)
			{
				const double encoded = static_cast<double>(channel) / 255;
				const double linear = (encoded <= 0.04045 ? encoded / 12.92 : std::pow((encoded + 0.055) / 1.055, 2.4)) * static_cast<double>(alpha) / 255;
				const double premultiplied = linear <= 0.0031308 ? linear * 12.92 : 1.055 * std::pow(linear, 1 / 2.4) - 0.055;
				values[alpha][channel] = static_cast<std::uint8_t>(std::lround(premultiplied * 255));
			}
		return values;
	}();
	for (std::size_t index = 0; index + 3 < rgba.size(); index += 4)
	{
		const auto& channels = table[rgba[index + 3]];
		for (std::size_t channel = 0; channel < 3; ++channel) rgba[index + channel] = channels[rgba[index + channel]];
	}
}
}
