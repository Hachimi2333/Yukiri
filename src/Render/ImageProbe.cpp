#include "ImageProbe.hpp"

namespace Yukiri::Detail
{
namespace
{
Error InvalidImage() { return {ErrorCode::ResourceLoading, "Unsupported or truncated PNG, JPEG, BMP, or ICO image."}; }
std::uint32_t Le32(std::span<const std::uint8_t> data, std::size_t offset)
{
	return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
		(static_cast<std::uint32_t>(data[offset + 2]) << 16) | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}
std::uint32_t Be32(std::span<const std::uint8_t> data, std::size_t offset)
{
	return (static_cast<std::uint32_t>(data[offset]) << 24) | (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
		(static_cast<std::uint32_t>(data[offset + 2]) << 8) | static_cast<std::uint32_t>(data[offset + 3]);
}
Result<void> ProbeDib(std::span<const std::uint8_t> data, bool icon)
{
	if (data.size() < 12) return std::unexpected(InvalidImage());
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	if (Le32(data, 0) == 12)
	{
		width = data[4] | (static_cast<std::uint32_t>(data[5]) << 8);
		height = data[6] | (static_cast<std::uint32_t>(data[7]) << 8);
	}
	else if (Le32(data, 0) >= 40 && data.size() >= 40)
	{
		width = Le32(data, 4);
		height = Le32(data, 8);
		if (height & 0x80000000U) height = ~height + 1;
	}
	else return std::unexpected(InvalidImage());
	return ValidateImageSize(width, icon ? height / 2 : height);
}
}
Result<void> ProbeRasterImage(std::span<const std::uint8_t> data)
{
	if (data.size() >= 24 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
		return ValidateImageSize(Be32(data, 16), Be32(data, 20));
	if (data.size() >= 26 && data[0] == 'B' && data[1] == 'M') return ProbeDib(data.subspan(14), false);
	if (data.size() >= 6 && data[0] == 0 && data[1] == 0 && data[2] == 1 && data[3] == 0)
	{
		const auto count = static_cast<std::size_t>(data[4] | (static_cast<unsigned>(data[5]) << 8));
		if (!count || count > (data.size() - 6) / 16) return std::unexpected(InvalidImage());
		std::uint64_t totalPixels = 0;
		for (std::size_t index = 0; index < count; ++index)
		{
			const auto entry = 6 + index * 16;
			const auto size = Le32(data, entry + 8);
			const auto offset = Le32(data, entry + 12);
			if (offset > data.size() || size > data.size() - offset || size < 12) return std::unexpected(InvalidImage());
			const auto bytes = data.subspan(offset, size);
			const bool png = bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G';
			if (png && bytes.size() < 24) return std::unexpected(InvalidImage());
			auto valid = png ? ValidateImageSize(Be32(bytes, 16), Be32(bytes, 20)) : ProbeDib(bytes, true);
			if (!valid) return valid;
			const auto width = png ? Be32(bytes, 16) : (Le32(bytes, 0) == 12 ?
				static_cast<unsigned>(bytes[4] | (static_cast<unsigned>(bytes[5]) << 8)) : Le32(bytes, 4));
			auto height = png ? Be32(bytes, 20) : (Le32(bytes, 0) == 12 ?
				static_cast<unsigned>(bytes[6] | (static_cast<unsigned>(bytes[7]) << 8)) : Le32(bytes, 8));
			if (!png) { if (height & 0x80000000U) height = ~height + 1; height /= 2; }
			totalPixels += static_cast<std::uint64_t>(width) * height;
			if (totalPixels > 64 * 1024 * 1024) return std::unexpected(InvalidImage());
		}
		return {};
	}
	if (data.size() >= 4 && data[0] == 0xFF && data[1] == 0xD8)
	{
		std::size_t offset = 2;
		while (offset + 3 < data.size())
		{
			if (data[offset++] != 0xFF) return std::unexpected(InvalidImage());
			while (offset < data.size() && data[offset] == 0xFF) ++offset;
			if (offset >= data.size()) break;
			const auto marker = data[offset++];
			if (marker == 0xD9 || marker == 0xDA || offset + 2 > data.size()) break;
			if (marker >= 0xD0 && marker <= 0xD7) continue;
			const auto length = static_cast<std::size_t>((static_cast<unsigned>(data[offset]) << 8) | data[offset + 1]);
			if (length < 2 || length > data.size() - offset) break;
			if (marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC)
			{
				if (length < 8) break;
				return ValidateImageSize((static_cast<unsigned>(data[offset + 5]) << 8) | data[offset + 6],
					(static_cast<unsigned>(data[offset + 3]) << 8) | data[offset + 4]);
			}
			offset += length;
		}
	}
	return std::unexpected(InvalidImage());
}
}
