#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Core/Geometry.hpp>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace Yukiri
{
namespace Detail { struct ImageData; class RenderDevice; }

class Image
{
public:
	Image() = default;
	[[nodiscard]] static Result<Image> Load(const std::filesystem::path& path);
	[[nodiscard]] static Result<Image> FromMemory(std::span<const std::uint8_t> encoded);
	[[nodiscard]] static Result<Image> FromRgba(std::uint32_t width, std::uint32_t height, std::span<const std::uint8_t> pixels);
	[[nodiscard]] Size GetSize() const noexcept;
	[[nodiscard]] std::span<const std::uint8_t> GetPixels() const noexcept;
	[[nodiscard]] bool IsVector() const noexcept;
	[[nodiscard]] explicit operator bool() const noexcept;
private:
	friend class Detail::RenderDevice;
	explicit Image(std::shared_ptr<const Detail::ImageData> data);
	std::shared_ptr<const Detail::ImageData> m_Data;
};
}
