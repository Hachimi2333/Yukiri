#include "RenderDevice.hpp"
#include "ImageData.hpp"
#include "ImageColor.hpp"
#include <algorithm>
#include <cmath>

namespace Yukiri::Detail
{
Result<std::uint32_t> RenderDevice::GetImageTexture(const Image& image, std::uint32_t width, std::uint32_t height)
{
	if (!image.m_Data) return std::unexpected(Error{ErrorCode::InvalidArgument, "An image is required."});
	if (!image.IsVector()) { width = image.m_Data->m_Pixels.m_Width; height = image.m_Data->m_Pixels.m_Height; }
	else
	{
		const auto natural = image.GetSize();
		const auto ratio = std::max(static_cast<float>(width) / natural.m_Width, static_cast<float>(height) / natural.m_Height);
		const auto rasterWidth = std::ceil(natural.m_Width * ratio);
		const auto rasterHeight = std::ceil(natural.m_Height * ratio);
		if (rasterWidth > 16384 || rasterHeight > 16384)
			return std::unexpected(Error{ErrorCode::ResourceLoading, "The SVG drawing scale exceeds the image limits."});
		width = static_cast<std::uint32_t>(rasterWidth);
		height = static_cast<std::uint32_t>(rasterHeight);
	}
	const auto key = std::tuple(image.m_Data->m_Id, width, height);
	if (auto* found = m_PublicImages.Find(key)) return found->m_Texture;
	ImagePixels raster;
	const ImagePixels* pixels = &image.m_Data->m_Pixels;
	if (image.IsVector() && (pixels->m_Width != width || pixels->m_Height != height))
	{
		auto generated = RasterizeSvg(image.m_Data->m_Svg, width, height);
		if (!generated) return std::unexpected(generated.error());
		raster = std::move(*generated);
		pixels = &raster;
	}
	auto created = CreateTexture(pixels->m_Width, pixels->m_Height, true);
	if (!created) return std::unexpected(created.error());
	auto& texture = m_Textures[*created];
	texture->m_Pixels = pixels->m_Bytes;
	PrepareImageUpload(texture->m_Pixels);
	texture->m_DiscardPixelsAfterUpload = true;
	m_PublicImages.Insert(key, image.m_Data, *created, texture->m_Pixels.size());
	return *created;
}
void RenderDevice::CollectImageTextures()
{
	m_PublicImages.Collect([this](std::uint32_t texture) { return m_Textures[texture].use_count() != 1; }, [this](std::uint32_t texture)
	{
		m_FreeTextureSlots.push_back(texture);
		m_Textures[texture].reset();
	});
}
}
