#include "TextEngine.hpp"
#include "Unicode.hpp"
#include <hb-ft.h>
#include FT_MULTIPLE_MASTERS_H
#include <algorithm>

namespace Yukiri::Detail
{
namespace
{
std::string FoldFamily(std::string family)
{
	for (auto& value : family) if (value >= 'A' && value <= 'Z') value = static_cast<char>(value - 'A' + 'a');
	return family;
}
void ConfigureFace(FT_Library library, FontFace& face, const Yukiri::Font& request)
{
	bool weightAxis = false;
	bool italicAxis = false;
	FT_MM_Var* axes = nullptr;
	if (!FT_Get_MM_Var(face.m_Face, &axes))
	{
		std::vector<FT_Fixed> coordinates(axes->num_axis);
		for (FT_UInt index = 0; index < axes->num_axis; ++index)
		{
			const auto& axis = axes->axis[index];
			coordinates[index] = axis.def;
			if (axis.tag == FT_MAKE_TAG('w', 'g', 'h', 't'))
			{
				coordinates[index] = std::clamp(static_cast<FT_Fixed>(request.m_Weight * 65536), axis.minimum, axis.maximum);
				weightAxis = true;
			}
			if (axis.tag == FT_MAKE_TAG('i', 't', 'a', 'l'))
			{
				coordinates[index] = std::clamp(static_cast<FT_Fixed>(request.m_Italic ? 65536 : 0), axis.minimum, axis.maximum);
				italicAxis = request.m_Italic;
			}
			if (axis.tag == FT_MAKE_TAG('s', 'l', 'n', 't') && request.m_Italic)
			{
				coordinates[index] = std::clamp(static_cast<FT_Fixed>(-12 * 65536), axis.minimum, axis.maximum);
				italicAxis = true;
			}
		}
		if (FT_Set_Var_Design_Coordinates(face.m_Face, axes->num_axis, coordinates.data())) weightAxis = italicAxis = false;
		FT_Done_MM_Var(library, axes);
	}
	face.m_SyntheticBold = !weightAxis && request.m_Weight >= 600 && !(face.m_Face->style_flags & FT_STYLE_FLAG_BOLD);
	face.m_SyntheticItalic = request.m_Italic && !italicAxis && !(face.m_Face->style_flags & FT_STYLE_FLAG_ITALIC);
	if (face.m_SyntheticItalic)
	{
		FT_Matrix transform{65536, 13930, 0, 65536};
		FT_Set_Transform(face.m_Face, &transform, nullptr);
		hb_font_set_synthetic_slant(face.m_Shaper, 0.2125f);
	}
	if (face.m_SyntheticBold) hb_font_set_synthetic_bold(face.m_Shaper, 0.03f, 0.03f, false);
	hb_ft_font_changed(face.m_Shaper);
}
}
FontFace* TextEngine::ResolveFace(const Yukiri::Font& requested)
{
	auto request = requested;
	request.m_Weight = std::clamp(request.m_Weight, 100, 900);
	if (request.m_Family.empty()) request.m_Family = m_FallbackFonts.front()->m_Source.m_Family;
	const auto folded = FoldFamily(request.m_Family);
	const auto key = std::tuple(folded, request.m_Weight, request.m_Italic);
	if (auto found = m_FaceChoices.find(key); found != m_FaceChoices.end()) return found->second;
	std::optional<FontSource> source;
	for (auto* font : m_FallbackFonts)
		if (font->m_Custom && FoldFamily(font->m_Source.m_Family) == folded) { source = font->m_Source; break; }
	if (!source) source = ResolveFont(request);
	if (!source)
	{
		for (auto* font : m_FallbackFonts)
			if (FoldFamily(font->m_Source.m_Family) == folded) { source = font->m_Source; break; }
	}
	if (source)
	{
		auto loaded = LoadFace(source->m_Path, source->m_Index);
		if (loaded)
		{
			auto face = std::move(*loaded);
			face->m_Source = *source;
			ConfigureFace(m_Library, *face, request);
			auto* observer = face.get();
			m_Fonts.push_back(std::move(face));
			m_FaceChoices.emplace(key, observer);
			return observer;
		}
	}
	m_FaceChoices.emplace(key, nullptr);
	return nullptr;
}
FontFace& TextEngine::ChooseFont(std::string_view grapheme, const Yukiri::Font& request)
{
	const auto codepoints = DecodeUtf8(grapheme);
	auto supported = [&](const FontFace& font)
	{
		return std::ranges::all_of(codepoints, [&](const auto codepoint)
		{
			return codepoint.m_Value == 0x200D || (codepoint.m_Value >= 0xFE00 && codepoint.m_Value <= 0xFE0F) ||
				FT_Get_Char_Index(font.m_Face, codepoint.m_Value) != 0;
		});
	};
	if (request != Yukiri::Font{})
		if (auto* preferred = ResolveFace(request); preferred && supported(*preferred)) return *preferred;
	for (auto* font : m_FallbackFonts)
	{
		if (!supported(*font)) continue;
		if (request.m_Weight != 400 || request.m_Italic)
		{
			auto fallback = request;
			fallback.m_Family = font->m_Source.m_Family;
			if (auto* styled = ResolveFace(fallback); styled && supported(*styled)) return *styled;
		}
		return *font;
	}
	return *m_FallbackFonts.front();
}
}
