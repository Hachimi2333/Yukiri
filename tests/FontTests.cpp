#include "Text/TextEngine.hpp"
#include "UI/WidgetTree.hpp"
#include <Yukiri/Controls/Label.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

TEST_CASE("Font descriptors separate shaping and preserve matching caret metrics", "[text][fonts]")
{
	auto engine = Yukiri::Detail::TextEngine::Create();
	REQUIRE(engine);
	const std::string text = "Yukiri AV 中文";
	const auto normal = (*engine)->Measure(text, 16);
	const Yukiri::Font bold{"", 700, false};
	const auto& shaped = (*engine)->Shape(text, 16, 1, bold);
	REQUIRE(shaped.m_Width > 0);
	REQUIRE_FALSE(shaped.m_Glyphs.empty());
	const auto boldFace = shaped.m_Glyphs.front().m_Font->m_Id;
	const auto* boldFont = shaped.m_Glyphs.front().m_Font;
	const auto carets = (*engine)->Carets(text, 16, 1, bold);
	REQUIRE(std::abs(carets.back().second - shaped.m_Width) < 0.01f);
	const auto& regular = (*engine)->Shape(text, 16);
	REQUIRE(regular.m_Glyphs.front().m_Font->m_Id != boldFace);
	if (regular.m_Glyphs.front().m_Font->m_Source.m_Path == boldFont->m_Source.m_Path)
		REQUIRE(regular.m_Glyphs.front().m_Font->m_Data == boldFont->m_Data);
	REQUIRE(std::abs(regular.m_Width - normal.m_Width) < 0.01f);
	const auto hits = (*engine)->GetShapeCacheHits();
	(*engine)->Shape(text, 16, 1, bold);
	REQUIRE((*engine)->GetShapeCacheHits() == hits + 1);
	const auto& italic = (*engine)->Shape(text, 16, 1, {"", 400, true});
	REQUIRE(italic.m_Glyphs.front().m_Font->m_Id != regular.m_Glyphs.front().m_Font->m_Id);
	REQUIRE((*engine)->Measure(text, 16, 1, {"Yukiri Missing Font Family", 400, false}).m_Width > 0);
	if (Yukiri::Detail::ResolveFont({"Consolas"}))
	{
		const auto narrow = (*engine)->Measure("iiii", 16, 1, {"Consolas"});
		const auto wide = (*engine)->Measure("WWWW", 16, 1, {"Consolas"});
		REQUIRE(std::abs(narrow.m_Width - wide.m_Width) < 0.1f);
	}
}

TEST_CASE("Widget fonts inherit without exposing platform types", "[style][fonts]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	auto& label = root->AddChild<Yukiri::Label>("Text");
	auto theme = Yukiri::Theme::Light();
	theme.m_Font = {"Consolas", 700, true};
	root->SetThemeOverride(theme);
	tree.m_MeasureText = [&](std::string_view, float, const Yukiri::Font& font)
	{
		REQUIRE(font == theme.m_Font);
		return Yukiri::Size{80, 20};
	};
	tree.SetRoot(std::move(root));
	tree.Layout({200, 50});
	REQUIRE(label.GetFont() == theme.m_Font);
	Yukiri::WidgetStyle style;
	style.m_Font = Yukiri::Font{"Segoe UI", 400, false};
	label.SetStyle(style);
	REQUIRE(label.GetFont() == *style.m_Font);
}

TEST_CASE("Long text carets are monotone and cover every grapheme", "[text][fonts]")
{
	auto engine = Yukiri::Detail::TextEngine::Create();
	REQUIRE(engine);
	const std::string text(10000, 'i');
	const auto carets = (*engine)->Carets(text, 14);
	REQUIRE(carets.size() == text.size() + 1);
	REQUIRE(carets.front().second == 0);
	REQUIRE(carets.back().first == text.size());
	for (std::size_t index = 1; index < carets.size(); ++index) REQUIRE(carets[index].second >= carets[index - 1].second);
	REQUIRE(std::abs(carets.back().second - (*engine)->Measure(text, 14).m_Width) < 0.01f);
}
