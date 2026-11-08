#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Core/Geometry.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Yukiri::Detail
{
class RenderDevice;
class DrawList;
struct Font
{
	~Font();
	std::vector<unsigned char> m_Data;
	FT_Face m_Face = nullptr;
	hb_font_t* m_Shaper = nullptr;
	std::uint32_t m_Id = 0;
};
struct ShapedGlyph
{
	Font* m_Font;
	std::uint32_t m_Glyph;
	std::uint32_t m_Cluster;
	float m_X;
	float m_Y;
	float m_Advance;
};
struct ShapedText
{
	std::vector<ShapedGlyph> m_Glyphs;
	float m_Width = 0;
	float m_Height = 0;
	float m_Ascent = 0;
};

class TextEngine
{
public:
	static Result<std::unique_ptr<TextEngine>> Create();
	~TextEngine();
	Result<void> LoadFont(const std::filesystem::path& path, long index = 0, bool prefer = true);
	ShapedText Shape(std::string_view text, float fontSize, float scale = 1);
	Size Measure(std::string_view text, float fontSize, float scale = 1);
	std::vector<std::pair<std::size_t, float>> Carets(std::string_view text, float fontSize, float scale = 1);
	Result<void> Draw(DrawList& list, std::string_view text, Point position, Color color, float fontSize, float scale, Rect clip);
	void SetDevice(RenderDevice* device) { m_Device = device; }

private:
	struct Glyph
	{
		std::uint32_t m_Texture = 0;
		Rect m_Uv;
		float m_Left = 0;
		float m_Top = 0;
		float m_Width = 0;
		float m_Height = 0;
	};
	struct Page { std::uint32_t m_Texture; int m_X = 1; int m_Y = 1; int m_RowHeight = 0; };
	TextEngine() = default;
	Result<Glyph> GetGlyph(Font& font, std::uint32_t glyph, unsigned pixels);
	Font& ChooseFont(std::string_view grapheme);
	FT_Library m_Library = nullptr;
	RenderDevice* m_Device = nullptr;
	std::vector<std::unique_ptr<Font>> m_Fonts;
	std::unordered_map<std::uint64_t, Glyph> m_Glyphs;
	std::vector<Page> m_Pages;
	std::uint32_t m_NextFont = 0;
};
}
