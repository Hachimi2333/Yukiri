#include <Yukiri/Render/Image.hpp>
#include "ImageData.hpp"
#include "ImageProbe.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <limits>

namespace Yukiri
{
namespace
{
std::atomic<std::uint64_t> NextImageId{1};
constexpr std::size_t MaximumEncodedBytes = 64 * 1024 * 1024;
}
Image::Image(std::shared_ptr<const Detail::ImageData> data) : m_Data(std::move(data)) {}
Image::operator bool() const noexcept { return m_Data != nullptr; }
Size Image::GetSize() const noexcept
{
	return m_Data ? Size{static_cast<float>(m_Data->m_Pixels.m_Width), static_cast<float>(m_Data->m_Pixels.m_Height)} : Size{};
}
bool Image::IsVector() const noexcept { return m_Data && !m_Data->m_Svg.empty(); }
std::span<const std::uint8_t> Image::GetPixels() const noexcept
{
	return m_Data ? std::span<const std::uint8_t>(m_Data->m_Pixels.m_Bytes) : std::span<const std::uint8_t>{};
}
Result<Image> Image::FromRgba(std::uint32_t width, std::uint32_t height, std::span<const std::uint8_t> pixels)
{
	auto valid = Detail::ValidateImageSize(width, height);
	if (!valid) return std::unexpected(Error{ErrorCode::InvalidArgument, valid.error().m_Message});
	if (pixels.size() != static_cast<std::size_t>(width) * height * 4)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "RGBA data must contain exactly four bytes per pixel."});
	auto data = std::make_shared<Detail::ImageData>();
	data->m_Id = NextImageId++;
	data->m_Pixels = {width, height, {pixels.begin(), pixels.end()}};
	return Image(std::move(data));
}
Result<Image> Image::Load(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) return std::unexpected(Error{ErrorCode::ResourceLoading, "Opening the image file failed."});
	const auto length = file.tellg();
	if (length <= 0 || length > static_cast<std::streamoff>(MaximumEncodedBytes))
		return std::unexpected(Error{ErrorCode::ResourceLoading, "The image file is empty or exceeds 64 MiB."});
	std::vector<std::uint8_t> encoded(static_cast<std::size_t>(length));
	file.seekg(0);
	if (!file.read(reinterpret_cast<char*>(encoded.data()), length))
		return std::unexpected(Error{ErrorCode::ResourceLoading, "Reading the image file failed."});
	return FromMemory(encoded);
}
Result<Image> Image::FromMemory(std::span<const std::uint8_t> encoded)
{
	if (encoded.empty() || encoded.size() > MaximumEncodedBytes)
		return std::unexpected(Error{ErrorCode::ResourceLoading, "Encoded image data must be between one byte and 64 MiB."});
	const std::string_view source(reinterpret_cast<const char*>(encoded.data()), encoded.size());
	const auto bom = source.starts_with("\xEF\xBB\xBF") ? std::size_t(3) : std::size_t(0);
	const auto first = source.find_first_not_of(" \t\r\n", bom);
	if (first != std::string_view::npos && source[first] == '<' &&
		source.substr(0, std::min<std::size_t>(4096, source.size())).find("<svg") != std::string_view::npos)
	{
		auto pixels = Detail::RasterizeSvg(source);
		if (!pixels) return std::unexpected(pixels.error());
		auto data = std::make_shared<Detail::ImageData>();
		data->m_Id = NextImageId++;
		data->m_Pixels = std::move(*pixels);
		data->m_Svg = source;
		return Image(std::move(data));
	}
	auto validSource = Detail::ProbeRasterImage(encoded);
	if (!validSource) return std::unexpected(validSource.error());
	auto* io = SDL_IOFromConstMem(encoded.data(), encoded.size());
	if (!io) return std::unexpected(Error{ErrorCode::ResourceLoading, SDL_GetError()});
	using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
	Surface decoded(IMG_Load_IO(io, true), SDL_DestroySurface);
	if (!decoded) return std::unexpected(Error{ErrorCode::ResourceLoading, SDL_GetError()});
	auto valid = Detail::ValidateImageSize(static_cast<std::uint32_t>(decoded->w), static_cast<std::uint32_t>(decoded->h));
	if (!valid) return std::unexpected(valid.error());
	Surface rgba(SDL_ConvertSurface(decoded.get(), SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
	if (!rgba) return std::unexpected(Error{ErrorCode::ResourceLoading, SDL_GetError()});
	auto data = std::make_shared<Detail::ImageData>();
	data->m_Id = NextImageId++;
	data->m_Pixels.m_Width = static_cast<std::uint32_t>(rgba->w);
	data->m_Pixels.m_Height = static_cast<std::uint32_t>(rgba->h);
	const auto rowBytes = static_cast<std::size_t>(rgba->w) * 4;
	data->m_Pixels.m_Bytes.resize(rowBytes * static_cast<std::size_t>(rgba->h));
	for (int row = 0; row < rgba->h; ++row)
		std::memcpy(data->m_Pixels.m_Bytes.data() + static_cast<std::size_t>(row) * rowBytes,
			static_cast<const std::uint8_t*>(rgba->pixels) + static_cast<std::size_t>(row) * static_cast<std::size_t>(rgba->pitch), rowBytes);
	return Image(std::move(data));
}
}

namespace Yukiri::Detail
{
Result<void> ValidateImageSize(std::uint32_t width, std::uint32_t height)
{
	if (!width || !height || width > 16384 || height > 16384 || static_cast<std::uint64_t>(width) * height > 64 * 1024 * 1024)
		return std::unexpected(Error{ErrorCode::ResourceLoading, "Image dimensions exceed the 16384-axis or 256 MiB decoded limits."});
	return {};
}
}
