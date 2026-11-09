#include "TextEngine.hpp"
#include "Render/RenderDevice.hpp"
#include "Render/DrawList.hpp"
#include <algorithm>
#include <cmath>
#include FT_SYNTHESIS_H

namespace Yukiri::Detail
{
Result<TextEngine::Glyph> TextEngine::GetGlyph(FontFace& font, std::uint32_t glyph, unsigned pixels)
{
	const auto key = (static_cast<std::uint64_t>(font.m_Id) << 48) |
		(static_cast<std::uint64_t>(pixels) << 32) | glyph;
	if (auto found = m_Glyphs.find(key); found != m_Glyphs.end()) return found->second;
	FT_Set_Pixel_Sizes(font.m_Face, 0, pixels);
	if (FT_Load_Glyph(font.m_Face, glyph, FT_LOAD_DEFAULT))
		return std::unexpected(Error{ErrorCode::FontLoading, "Loading a font glyph failed."});
	if (font.m_SyntheticBold) FT_GlyphSlot_Embolden(font.m_Face->glyph);
	if (FT_Render_Glyph(font.m_Face->glyph, FT_RENDER_MODE_NORMAL))
		return std::unexpected(Error{ErrorCode::FontLoading, "Rasterizing a font glyph failed."});
	const auto& bitmap = font.m_Face->glyph->bitmap;
	if (bitmap.width && bitmap.rows && bitmap.pixel_mode != FT_PIXEL_MODE_GRAY &&
		bitmap.pixel_mode != FT_PIXEL_MODE_MONO && bitmap.pixel_mode != FT_PIXEL_MODE_BGRA)
		return std::unexpected(Error{ErrorCode::FontLoading, "The font uses an unsupported glyph bitmap format."});
	Glyph result;
	result.m_Left = static_cast<float>(font.m_Face->glyph->bitmap_left);
	result.m_Top = static_cast<float>(font.m_Face->glyph->bitmap_top);
	result.m_Width = static_cast<float>(bitmap.width);
	result.m_Height = static_cast<float>(bitmap.rows);
	if (!bitmap.width || !bitmap.rows) { m_Glyphs.emplace(key, result); return result; }
	constexpr int AtlasSize = 2048;
	const int width = static_cast<int>(bitmap.width);
	const int height = static_cast<int>(bitmap.rows);
	if (width + 2 > AtlasSize || height + 2 > AtlasSize)
		return std::unexpected(Error{ErrorCode::FontLoading, "The glyph exceeds the atlas page size."});
	if (!m_Pages.empty() && m_Pages.back().m_X + width + 1 >= AtlasSize)
	{
		auto& page = m_Pages.back();
		page.m_X = 1;
		page.m_Y += page.m_RowHeight + 1;
		page.m_RowHeight = 0;
	}
	if (m_Pages.empty() || m_Pages.back().m_Y + height + 1 >= AtlasSize)
	{
		auto texture = m_Device->CreateTexture(AtlasSize, AtlasSize);
		if (!texture) return std::unexpected(texture.error());
		m_Pages.push_back({*texture});
	}
	auto& page = m_Pages.back();
	auto& texture = *m_Device->m_Textures[page.m_Texture];
	for (int y = 0; y < height; ++y)
	{
		const auto* row = bitmap.buffer + (bitmap.pitch >= 0 ? y * bitmap.pitch : (height - 1 - y) * -bitmap.pitch);
		for (int x = 0; x < width; ++x)
		{
			unsigned char value = 0;
			if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) value = (row[x / 8] & (0x80 >> (x % 8))) ? 255 : 0;
			else if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) value = row[x * 4 + 3];
			else value = row[x];
			texture.m_Pixels[static_cast<std::size_t>(page.m_Y + y) * AtlasSize + static_cast<std::size_t>(page.m_X + x)] = value;
		}
	}
	texture.MarkDirty(page.m_X, page.m_Y, bitmap.width, bitmap.rows);
	result.m_Texture = page.m_Texture;
	result.m_Uv = {static_cast<float>(page.m_X) / AtlasSize, static_cast<float>(page.m_Y) / AtlasSize,
		static_cast<float>(width) / AtlasSize, static_cast<float>(height) / AtlasSize};
	page.m_X += width + 1;
	page.m_RowHeight = std::max(page.m_RowHeight, height);
	m_Glyphs.emplace(key, result);
	return result;
}

Result<void> TextEngine::Draw(DrawList& list, std::string_view text, Point position, Color color, float fontSize, float scale, Rect clip, const Yukiri::Font& font)
{
	if (!m_Device) return std::unexpected(Error{ErrorCode::GraphicsInitialization, "Text drawing requires an initialized graphics device."});
	const auto& shaped = Shape(text, fontSize, scale, font);
	const auto pixels = static_cast<unsigned>(std::clamp(std::round(fontSize * scale), 1.0f, 512.0f));
	for (const auto& item : shaped.m_Glyphs)
	{
		auto glyph = GetGlyph(*item.m_Font, item.m_Glyph, pixels);
		if (!glyph) return std::unexpected(glyph.error());
		list.Add({std::round((position.m_X + item.m_X) * scale + glyph->m_Left) / scale,
			std::round((position.m_Y + shaped.m_Ascent + item.m_Y) * scale - glyph->m_Top) / scale,
			glyph->m_Width / scale, glyph->m_Height / scale}, color, clip, 0, 0, glyph->m_Texture, glyph->m_Uv, DrawKind::Glyph);
	}
	return {};
}
}
