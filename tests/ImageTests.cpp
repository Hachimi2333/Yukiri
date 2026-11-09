#include <Yukiri/Controls/ImageView.hpp>
#include "Render/ImageCache.hpp"
#include "Render/ImageProbe.hpp"
#include "Render/ImageColor.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <fstream>
#include <random>
#include <cmath>

namespace
{
std::span<const std::uint8_t> Bytes(std::string_view value)
{
	return {reinterpret_cast<const std::uint8_t*>(value.data()), value.size()};
}
}

TEST_CASE("Image decoding supports the selected raster formats and embedded project resources", "[image]")
{
	const std::filesystem::path root(YUKIRI_TEST_SOURCE_DIR);
	for (const auto* name : {"sample.png", "sample.jpg", "sample.bmp", "sample.ico"})
	{
		auto image = Yukiri::Image::Load(root / "third_party/SDL_image/test" / name);
		INFO(name);
		REQUIRE(image);
		REQUIRE(image->GetSize().m_Width > 0);
		REQUIRE(image->GetSize().m_Height > 0);
		REQUIRE(image->GetPixels().size() == static_cast<std::size_t>(image->GetSize().m_Width * image->GetSize().m_Height) * 4);
		REQUIRE_FALSE(image->IsVector());
	}
	REQUIRE(Yukiri::Image::Load(root / "assets/Yukiri.ico"));
	auto logo = Yukiri::Image::Load(root / "assets/Yukiri.svg");
	REQUIRE(logo);
	REQUIRE(logo->IsVector());
}

TEST_CASE("SVG rasterization preserves straight alpha and scales independently of the display", "[image]")
{
	const std::string svg = "<svg xmlns='http://www.w3.org/2000/svg' width='4' height='4'><rect x='1' y='1' width='2' height='2' fill='#ff0000' opacity='.5'/></svg>";
	auto image = Yukiri::Image::FromMemory(Bytes(svg));
	REQUIRE(image);
	REQUIRE(image->GetSize().m_Width == 4);
	REQUIRE(image->GetPixels()[3] == 0);
	REQUIRE(image->GetPixels()[(1 * 4 + 1) * 4] >= 254);
	REQUIRE(image->GetPixels()[(1 * 4 + 1) * 4 + 3] >= 127);
	auto scaled = Yukiri::Detail::RasterizeSvg(svg, 8, 8);
	REQUIRE(scaled);
	REQUIRE(scaled->m_Bytes.size() == 8 * 8 * 4);
	REQUIRE(scaled->m_Bytes[(3 * 8 + 3) * 4] >= 254);
	Yukiri::ImageView view(*image);
	view.Measure({2, 2});
	REQUIRE(view.GetDesiredSize().m_Width == 2);
}

TEST_CASE("Image creation validates dimensions, input lengths, and Unicode file paths", "[image]")
{
	std::array<std::uint8_t, 4> pixel{255, 100, 0, 128};
	auto rgba = Yukiri::Image::FromRgba(1, 1, pixel);
	REQUIRE(rgba);
	REQUIRE(rgba->GetPixels()[3] == 128);
	REQUIRE_FALSE(Yukiri::Image::FromRgba(0, 1, pixel));
	REQUIRE_FALSE(Yukiri::Image::FromRgba(1, 2, pixel));
	REQUIRE_FALSE(Yukiri::Image::FromMemory({}));
	REQUIRE_FALSE(Yukiri::Image::FromMemory(Bytes("invalid")));
	REQUIRE_FALSE(Yukiri::Image::FromMemory(Bytes("<svg width='9999999' height='4'></svg>")));
	std::array<std::uint8_t, 24> png{};
	png[0] = 0x89; png[1] = 'P'; png[2] = 'N'; png[3] = 'G';
	png[16] = 0x7F; png[20] = 0x7F;
	REQUIRE_FALSE(Yukiri::Detail::ProbeRasterImage(png));
	const auto path = std::filesystem::temp_directory_path() / (std::filesystem::path(u8"Yukiri-图片-") += std::to_string(std::random_device{}()) + ".svg");
	struct Cleanup
	{
		std::filesystem::path m_Path;
		~Cleanup() { std::error_code error; std::filesystem::remove(m_Path, error); }
	} cleanup{path};
	{
		std::ofstream output(path, std::ios::binary);
		output << "<svg width='1' height='1'><rect width='1' height='1' fill='blue'/></svg>";
	}
	REQUIRE(Yukiri::Image::Load(path));
}

TEST_CASE("Image cache evicts least recent resources while honoring frame pins and owner expiry", "[image][resources]")
{
	Yukiri::Detail::ImageCache cache(8);
	auto owner = std::make_shared<Yukiri::Detail::ImageData>();
	cache.Insert({1, 1, 1}, owner, 1, 4);
	cache.Insert({2, 1, 1}, owner, 2, 4);
	cache.Insert({3, 1, 1}, owner, 3, 4);
	REQUIRE(cache.Find({1, 1, 1}));
	std::vector<std::uint32_t> retired;
	cache.Collect([](auto) { return false; }, [&](auto id) { retired.push_back(id); });
	REQUIRE(retired == std::vector<std::uint32_t>{2});
	owner.reset();
	cache.Collect([](auto id) { return id == 1; }, [&](auto id) { retired.push_back(id); });
	REQUIRE(cache.GetCount() == 1);
	REQUIRE(cache.Find({1, 1, 1}));
	cache.Collect([](auto) { return false; }, [&](auto id) { retired.push_back(id); });
	REQUIRE(cache.GetCount() == 0);
	for (std::uint32_t id = 0; id < 2000; ++id) cache.Insert({id, 1, 1}, {}, id, 4);
	cache.Collect([](auto) { return false; }, [&](auto) {});
	REQUIRE(cache.GetCount() == 0);
}

TEST_CASE("Image upload premultiplies linear color before sRGB texture filtering", "[image][color]")
{
	std::array<std::uint8_t, 8> pixels{255, 0, 0, 128, 255, 200, 100, 0};
	Yukiri::Detail::PrepareImageUpload(pixels);
	const double sampledRed = std::pow((static_cast<double>(pixels[0]) / 255 + 0.055) / 1.055, 2.4);
	REQUIRE(std::abs(sampledRed - 128.0 / 255) < 0.005);
	REQUIRE(pixels[3] == 128);
	REQUIRE(pixels[4] == 0);
	REQUIRE(pixels[5] == 0);
	REQUIRE(pixels[6] == 0);
}
