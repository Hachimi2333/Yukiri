#include "ImageData.hpp"
#include <lunasvg.h>
#include <cmath>
#include <cstring>

namespace Yukiri::Detail
{
Result<ImagePixels> RasterizeSvg(std::string_view source, std::uint32_t width, std::uint32_t height)
{
	auto document = lunasvg::Document::loadFromData(source.data(), source.size());
	if (!document) return std::unexpected(Error{ErrorCode::ResourceLoading, "Parsing the SVG document failed."});
	const auto naturalWidth = document->width();
	const auto naturalHeight = document->height();
	if (!std::isfinite(naturalWidth) || !std::isfinite(naturalHeight) || naturalWidth <= 0 || naturalHeight <= 0 ||
		naturalWidth > 16384 || naturalHeight > 16384)
		return std::unexpected(Error{ErrorCode::ResourceLoading, "SVG intrinsic dimensions are invalid."});
	if (!width) width = static_cast<std::uint32_t>(std::ceil(naturalWidth));
	if (!height) height = static_cast<std::uint32_t>(std::ceil(naturalHeight));
	auto valid = ValidateImageSize(width, height);
	if (!valid) return std::unexpected(valid.error());
	auto bitmap = document->renderToBitmap(static_cast<int>(width), static_cast<int>(height));
	if (bitmap.isNull()) return std::unexpected(Error{ErrorCode::ResourceLoading, "Rasterizing the SVG document failed."});
	bitmap.convertToRGBA();
	ImagePixels pixels{width, height, {}};
	const auto rowBytes = static_cast<std::size_t>(width) * 4;
	pixels.m_Bytes.resize(rowBytes * height);
	for (std::uint32_t row = 0; row < height; ++row)
		std::memcpy(pixels.m_Bytes.data() + row * rowBytes, bitmap.data() + static_cast<std::size_t>(row) * bitmap.stride(), rowBytes);
	return pixels;
}
}
