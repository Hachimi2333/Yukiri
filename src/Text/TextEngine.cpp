#include "TextEngine.hpp"
#include "Unicode.hpp"
#include "Platform/FontDiscovery.hpp"
#include <hb-ft.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

namespace Yukiri::Detail
{
Font::~Font()
{
	if (m_Shaper) hb_font_destroy(m_Shaper);
	if (m_Face) FT_Done_Face(m_Face);
}
TextEngine::~TextEngine()
{
	m_Fonts.clear();
	if (m_Library) FT_Done_FreeType(m_Library);
}
Result<std::unique_ptr<TextEngine>> TextEngine::Create()
{
	auto engine = std::unique_ptr<TextEngine>(new TextEngine());
	if (FT_Init_FreeType(&engine->m_Library))
		return std::unexpected(Error{ErrorCode::FontLoading, "FreeType initialization failed."});
	for (const auto& source : DiscoverFonts())
	{
		auto loaded = engine->LoadFont(source.m_Path, source.m_Index, false);
		(void)loaded;
	}
	if (engine->m_Fonts.empty())
		return std::unexpected(Error{ErrorCode::FontLoading, "No usable system fonts were found."});
	return engine;
}
Result<void> TextEngine::LoadFont(const std::filesystem::path& path, long index, bool prefer)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) return std::unexpected(Error{ErrorCode::FontLoading, "Cannot open the requested font file."});
	const auto length = file.tellg();
	if (length <= 0 || length > std::numeric_limits<FT_Long>::max())
		return std::unexpected(Error{ErrorCode::FontLoading, "The font file size is invalid."});
	auto font = std::make_unique<Font>();
	font->m_Data.resize(static_cast<std::size_t>(length));
	file.seekg(0);
	if (!file.read(reinterpret_cast<char*>(font->m_Data.data()), static_cast<std::streamsize>(length)))
		return std::unexpected(Error{ErrorCode::FontLoading, "Reading the font file failed."});
	if (FT_New_Memory_Face(m_Library, font->m_Data.data(), static_cast<FT_Long>(font->m_Data.size()), index, &font->m_Face))
		return std::unexpected(Error{ErrorCode::FontLoading, "FreeType could not load the requested font face."});
	font->m_Shaper = hb_ft_font_create_referenced(font->m_Face);
	font->m_Id = m_NextFont++;
	if (prefer) m_Fonts.insert(m_Fonts.begin(), std::move(font)); else m_Fonts.push_back(std::move(font));
	return {};
}
Font& TextEngine::ChooseFont(std::string_view grapheme)
{
	const auto codepoints = DecodeUtf8(grapheme);
	for (const auto& font : m_Fonts)
	{
		const bool supported = std::ranges::all_of(codepoints, [&](const auto codepoint)
		{
			return codepoint.m_Value == 0x200D || (codepoint.m_Value >= 0xFE00 && codepoint.m_Value <= 0xFE0F) ||
				FT_Get_Char_Index(font->m_Face, codepoint.m_Value) != 0;
		});
		if (supported) return *font;
	}
	return *m_Fonts.front();
}
ShapedText TextEngine::Shape(std::string_view input, float fontSize, float scale)
{
	const std::string text = SanitizeUtf8(input);
	ShapedText shaped;
	shaped.m_Height = fontSize * 1.5f;
	shaped.m_Ascent = fontSize * 1.1f;
	if (text.empty()) return shaped;
	const auto boundaries = GraphemeBoundaries(text);
	const auto pixels = static_cast<unsigned>(std::clamp(std::round(fontSize * scale), 1.0f, 512.0f));
	const float unit = 1.0f / (64.0f * scale);
	std::size_t cluster = 0;
	while (cluster + 1 < boundaries.size())
	{
		const auto start = boundaries[cluster];
		auto& font = ChooseFont(std::string_view(text).substr(start, boundaries[cluster + 1] - start));
		std::size_t next = cluster + 1;
		while (next + 1 < boundaries.size() && &ChooseFont(std::string_view(text).substr(boundaries[next], boundaries[next + 1] - boundaries[next])) == &font) ++next;
		FT_Set_Pixel_Sizes(font.m_Face, 0, pixels);
		hb_ft_font_changed(font.m_Shaper);
		shaped.m_Ascent = std::max(shaped.m_Ascent, static_cast<float>(font.m_Face->size->metrics.ascender) * unit);
		shaped.m_Height = std::max(shaped.m_Height, static_cast<float>(font.m_Face->size->metrics.height) * unit);
		hb_buffer_t* buffer = hb_buffer_create();
		hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
		hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), static_cast<unsigned>(start), static_cast<int>(boundaries[next] - start));
		hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
		hb_buffer_guess_segment_properties(buffer);
		hb_shape(font.m_Shaper, buffer, nullptr, 0);
		unsigned count = 0;
		const auto* infos = hb_buffer_get_glyph_infos(buffer, &count);
		const auto* positions = hb_buffer_get_glyph_positions(buffer, &count);
		for (unsigned i = 0; i < count; ++i)
		{
			const float advance = static_cast<float>(positions[i].x_advance) * unit;
			shaped.m_Glyphs.push_back({&font, infos[i].codepoint, infos[i].cluster,
				shaped.m_Width + static_cast<float>(positions[i].x_offset) * unit,
				-static_cast<float>(positions[i].y_offset) * unit, advance});
			shaped.m_Width += advance;
		}
		hb_buffer_destroy(buffer);
		cluster = next;
	}
	return shaped;
}
Size TextEngine::Measure(std::string_view text, float fontSize, float scale)
{
	const auto shaped = Shape(text, fontSize, scale);
	return {shaped.m_Width, shaped.m_Height};
}
std::vector<std::pair<std::size_t, float>> TextEngine::Carets(std::string_view text, float fontSize, float scale)
{
	const auto shaped = Shape(text, fontSize, scale);
	const auto boundaries = GraphemeBoundaries(text);
	std::vector<std::pair<std::size_t, float>> result;
	for (const auto boundary : boundaries)
	{
		float position = shaped.m_Width;
		for (std::size_t i = 0; i < shaped.m_Glyphs.size();)
		{
			const auto start = shaped.m_Glyphs[i].m_Cluster;
			const float x = shaped.m_Glyphs[i].m_X;
			float advance = 0;
			std::size_t next = i;
			while (next < shaped.m_Glyphs.size() && shaped.m_Glyphs[next].m_Cluster == start) advance += shaped.m_Glyphs[next++].m_Advance;
			const auto end = next == shaped.m_Glyphs.size() ? text.size() : shaped.m_Glyphs[next].m_Cluster;
			if (boundary >= start && boundary < end)
			{
				const auto first = std::ranges::lower_bound(boundaries, start);
				const auto last = std::ranges::lower_bound(boundaries, end);
				const auto current = std::ranges::lower_bound(boundaries, boundary);
				position = x + advance * static_cast<float>(current - first) / static_cast<float>(std::max<std::ptrdiff_t>(1, last - first));
				break;
			}
			i = next;
		}
		result.emplace_back(boundary, position);
	}
	return result;
}
}
