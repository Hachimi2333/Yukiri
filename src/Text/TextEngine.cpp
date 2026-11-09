#include "TextEngine.hpp"
#include "Unicode.hpp"
#include "Platform/FontDiscovery.hpp"
#include <hb-ft.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <cmath>

namespace Yukiri::Detail
{
FontFace::~FontFace()
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
		if (loaded && !source.m_Family.empty()) engine->m_FallbackFonts.back()->m_Source.m_Family = source.m_Family;
	}
	if (engine->m_Fonts.empty())
		return std::unexpected(Error{ErrorCode::FontLoading, "No usable system fonts were found."});
	return engine;
}
Result<std::unique_ptr<FontFace>> TextEngine::LoadFace(const std::filesystem::path& path, long index, bool reuseData)
{
	auto font = std::make_unique<FontFace>();
	if (reuseData) font->m_Data = m_FontBytes[path].lock();
	if (!font->m_Data)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file) return std::unexpected(Error{ErrorCode::FontLoading, "Cannot open the requested font file."});
		const auto length = file.tellg();
		if (length <= 0 || length > std::numeric_limits<FT_Long>::max())
			return std::unexpected(Error{ErrorCode::FontLoading, "The font file size is invalid."});
		auto bytes = std::make_shared<std::vector<unsigned char>>(static_cast<std::size_t>(length));
		file.seekg(0);
		if (!file.read(reinterpret_cast<char*>(bytes->data()), static_cast<std::streamsize>(length)))
			return std::unexpected(Error{ErrorCode::FontLoading, "Reading the font file failed."});
		font->m_Data = bytes;
		m_FontBytes[path] = std::move(bytes);
	}
	if (FT_New_Memory_Face(m_Library, font->m_Data->data(), static_cast<FT_Long>(font->m_Data->size()), index, &font->m_Face))
		return std::unexpected(Error{ErrorCode::FontLoading, "FreeType could not load the requested font face."});
	font->m_Shaper = hb_ft_font_create_referenced(font->m_Face);
	font->m_Id = m_NextFont++;
	font->m_Source = {path, index, font->m_Face->family_name ? font->m_Face->family_name : ""};
	return font;
}
Result<void> TextEngine::LoadFont(const std::filesystem::path& path, long index, bool prefer)
{
	auto loaded = LoadFace(path, index, !prefer);
	if (!loaded) return std::unexpected(loaded.error());
	auto font = std::move(*loaded);
	font->m_Custom = prefer;
	m_ShapeIndex.clear();
	m_Shapes.clear();
	m_CacheBytes = 0;
	m_FaceChoices.clear();
	if (prefer) m_FallbackFonts.insert(m_FallbackFonts.begin(), font.get()); else m_FallbackFonts.push_back(font.get());
	m_Fonts.push_back(std::move(font));
	return {};
}
std::size_t TextEngine::ShapeHash::operator()(const ShapeKey& key) const noexcept
{
	auto hash = std::hash<std::string>{}(key.m_Text);
	hash ^= std::hash<float>{}(key.m_FontSize) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= std::hash<float>{}(key.m_Scale) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= std::hash<std::string>{}(key.m_Font.m_Family) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= std::hash<int>{}(key.m_Font.m_Weight) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash ^ std::hash<bool>{}(key.m_Font.m_Italic);
}
const ShapedText& TextEngine::Shape(std::string_view input, float fontSize, float scale, const Yukiri::Font& font)
{
	fontSize = std::isfinite(fontSize) ? std::clamp(fontSize, 1.0f, 512.0f) : 14;
	scale = std::isfinite(scale) ? std::clamp(scale, 0.25f, 8.0f) : 1;
	ShapeKey key{std::string(input), fontSize, scale, font};
	if (const auto found = m_ShapeIndex.find(key); found != m_ShapeIndex.end())
	{
		++m_CacheHits;
		m_Shapes.splice(m_Shapes.begin(), m_Shapes, found->second);
		return found->second->m_Shaped;
	}
	auto shaped = ShapeUncached(input, fontSize, scale, font);
	constexpr std::size_t Budget = 8 * 1024 * 1024;
	const auto bytes = sizeof(CachedShape) + 2 * key.m_Text.capacity() + 2 * key.m_Font.m_Family.capacity() + shaped.m_Glyphs.capacity() * sizeof(ShapedGlyph) + 128;
	if (bytes > Budget) { m_Uncached = std::move(shaped); return m_Uncached; }
	while (m_CacheBytes + bytes > Budget)
	{
		m_CacheBytes -= m_Shapes.back().m_Bytes;
		m_ShapeIndex.erase(m_Shapes.back().m_Key);
		m_Shapes.pop_back();
	}
	m_Shapes.push_front({std::move(key), std::move(shaped), bytes});
	m_ShapeIndex.emplace(m_Shapes.front().m_Key, m_Shapes.begin());
	m_CacheBytes += bytes;
	return m_Shapes.front().m_Shaped;
}
ShapedText TextEngine::ShapeUncached(std::string_view input, float fontSize, float scale, const Yukiri::Font& request)
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
		auto& font = ChooseFont(std::string_view(text).substr(start, boundaries[cluster + 1] - start), request);
		std::size_t next = cluster + 1;
		while (next + 1 < boundaries.size() && &ChooseFont(std::string_view(text).substr(boundaries[next], boundaries[next + 1] - boundaries[next]), request) == &font) ++next;
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
Size TextEngine::Measure(std::string_view text, float fontSize, float scale, const Yukiri::Font& font)
{
	const auto& shaped = Shape(text, fontSize, scale, font);
	return {shaped.m_Width, shaped.m_Height};
}
std::vector<std::pair<std::size_t, float>> TextEngine::Carets(std::string_view text, float fontSize, float scale, const Yukiri::Font& font)
{
	const auto& shaped = Shape(text, fontSize, scale, font);
	const auto boundaries = GraphemeBoundaries(text);
	std::vector<std::pair<std::size_t, float>> result;
	result.reserve(boundaries.size());
	std::size_t glyph = 0;
	std::size_t next = 0;
	std::size_t start = 0;
	std::size_t end = 0;
	float x = 0;
	float advance = 0;
	std::size_t firstBoundary = 0;
	std::size_t lastBoundary = 0;
	auto advanceCluster = [&]
	{
		x += advance;
		glyph = next;
		if (glyph == shaped.m_Glyphs.size()) return false;
		start = shaped.m_Glyphs[glyph].m_Cluster;
		advance = 0;
		next = glyph;
		while (next < shaped.m_Glyphs.size() && shaped.m_Glyphs[next].m_Cluster == start) advance += shaped.m_Glyphs[next++].m_Advance;
		end = next == shaped.m_Glyphs.size() ? text.size() : shaped.m_Glyphs[next].m_Cluster;
		while (firstBoundary < boundaries.size() && boundaries[firstBoundary] < start) ++firstBoundary;
		lastBoundary = firstBoundary;
		while (lastBoundary < boundaries.size() && boundaries[lastBoundary] < end) ++lastBoundary;
		return true;
	};
	bool hasCluster = advanceCluster();
	for (std::size_t boundaryIndex = 0; boundaryIndex < boundaries.size(); ++boundaryIndex)
	{
		const auto boundary = boundaries[boundaryIndex];
		float position = shaped.m_Width;
		while (hasCluster && boundary >= end) hasCluster = advanceCluster();
		if (hasCluster && boundary >= start && boundary < end)
		{
			position = x + advance * static_cast<float>(boundaryIndex - firstBoundary) / static_cast<float>(std::max<std::size_t>(1, lastBoundary - firstBoundary));
		}
		result.emplace_back(boundary, position);
	}
	return result;
}
}
