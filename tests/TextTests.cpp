#include "Text/TextEditor.hpp"
#include "Text/TextEngine.hpp"
#include "UI/WidgetTree.hpp"
#include <Yukiri/Controls/TextBox.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace std::string_literals;

TEST_CASE("Grapheme editing preserves CJK combining characters and joined emoji", "[text]")
{
	Yukiri::Detail::TextEditor editor;
	editor.SetText("A中e\xCC\x81👩‍💻");
	const auto original = editor.m_Text;
	editor.Backspace();
	REQUIRE(editor.m_Text == "A中e\xCC\x81");
	editor.Undo();
	REQUIRE(editor.m_Text == original);
	REQUIRE(editor.m_Caret == original.size());
	REQUIRE(editor.m_Anchor == editor.m_Caret);
	editor.Redo();
	editor.Backspace();
	REQUIRE(editor.m_Text == "A中");
	editor.MoveLeft(false);
	REQUIRE(editor.m_Caret == 1);
	editor.Delete();
	REQUIRE(editor.m_Text == "A");
}

TEST_CASE("Selection replacement and undo retain UTF-8 boundaries", "[text]")
{
	Yukiri::Detail::TextEditor editor;
	editor.SetText("你好 world");
	editor.Select(0, 6);
	editor.Insert("Yukiri");
	REQUIRE(editor.m_Text == "Yukiri world");
	editor.Undo();
	REQUIRE(editor.GetSelection() == "你好");
	editor.MoveRight(false);
	REQUIRE(editor.m_Caret == 6);
	editor.Select(1, 5);
	REQUIRE(editor.m_Anchor == 0);
	REQUIRE(editor.m_Caret == 3);
}

TEST_CASE("Invalid UTF-8 is normalized and single-line editing filters line breaks", "[text]")
{
	Yukiri::Detail::TextEditor editor;
	editor.SetText("A\xFF\nB\tC"s);
	REQUIRE(editor.m_Text == "A\xEF\xBF\xBD B C");
	for (const auto point : Yukiri::Detail::DecodeUtf8(editor.m_Text))
		REQUIRE(point.m_Value <= 0x10FFFF);
	REQUIRE(Yukiri::Detail::GraphemeBoundaries("") == std::vector<std::size_t>{0});
}

TEST_CASE("Font fallback shaping and caret positions work without a window", "[text][fonts]")
{
	auto engine = Yukiri::Detail::TextEngine::Create();
	REQUIRE(engine);
	const auto shaped = (*engine)->Shape("Yukiri 中文 AV ffi", 16);
	REQUIRE(shaped.m_Width > 80);
	REQUIRE(shaped.m_Height >= 16);
	REQUIRE_FALSE(shaped.m_Glyphs.empty());
	const auto latin = (*engine)->Shape("Yukiri AV ffi", 16);
	for (const auto& glyph : latin.m_Glyphs) REQUIRE(glyph.m_Glyph != 0);
	const auto cjk = (*engine)->Shape("中文", 16);
	if (std::ranges::all_of(cjk.m_Glyphs, [](const auto& glyph) { return glyph.m_Glyph != 0; }))
	{
		for (const auto& glyph : shaped.m_Glyphs) REQUIRE(glyph.m_Glyph != 0);
	}
	else WARN("No system CJK font is installed; CJK fallback coverage requires that font fixture.");
	const auto carets = (*engine)->Carets("AV ffi", 16);
	REQUIRE(carets.front().second == Catch::Approx(0).margin(1));
	REQUIRE(carets.back().second == Catch::Approx((*engine)->Measure("AV ffi", 16).m_Width));
	for (std::size_t i = 1; i < carets.size(); ++i) REQUIRE(carets[i].second >= carets[i - 1].second);
	const float baseline = (*engine)->Measure("Yukiri 中文", 16, 1).m_Width;
	for (const float scale : {1.0f, 1.5f, 2.0f})
	{
		const auto measured = (*engine)->Measure("Yukiri 中文", 16, scale);
		REQUIRE(measured.m_Width == Catch::Approx(baseline).margin(4));
		REQUIRE((*engine)->Carets("Yukiri 中文", 16, scale).back().second == Catch::Approx(measured.m_Width));
	}
	REQUIRE_FALSE((*engine)->LoadFont("missing-font-file.ttf"));
}

TEST_CASE("IME preedit is separate from committed text and selection", "[text][ime]")
{
	Yukiri::Detail::WidgetTree tree;
	auto box = std::make_unique<Yukiri::TextBox>("original");
	auto* editor = box.get();
	tree.SetRoot(std::move(box));
	tree.SetFocus(editor);
	editor->SelectAll();
	tree.TextEditing("zhong", 5, 0);
	REQUIRE(editor->GetText() == "original");
	tree.KeyDown({Yukiri::Key::Tab});
	REQUIRE(tree.m_Focus == editor);
	tree.TextInput("中文");
	REQUIRE(editor->GetText() == "中文");
	tree.KeyDown({Yukiri::Key::Z, {false, true, false}});
	REQUIRE(editor->GetText() == "original");
	tree.TextEditing("cancel", 6, 0);
	tree.TextEditing("", 0, 0);
	REQUIRE(editor->GetText() == "original");
	editor->SetReadOnly(true);
	tree.TextInput("ignored");
	REQUIRE(editor->GetText() == "original");
}
