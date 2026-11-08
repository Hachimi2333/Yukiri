#include "Platform/FontDiscovery.hpp"
#include "Text/TextEditor.hpp"
#include "Text/TextComposition.hpp"
#include "Text/TextEngine.hpp"
#include "UI/WidgetTree.hpp"
#include <Yukiri/Controls/TextBox.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>

using namespace std::string_literals;

TEST_CASE("Preedit display replaces selection without mutating committed text", "[text][ime]")
{
	Yukiri::Detail::TextEditor editor;
	editor.SetText("A原文Z");
	editor.Select(1, 7);
	Yukiri::Detail::TextComposition composition;
	composition.Update("ni", 2, 0);
	REQUIRE(composition.BuildDisplay(editor).m_Text == "AniZ");
	composition.Update("ni'hao", 3, 3);
	const auto display = composition.BuildDisplay(editor);
	REQUIRE(display.m_Text == "Ani'haoZ");
	REQUIRE(display.m_Caret == 4);
	REQUIRE(display.m_SelectionStart == 4);
	REQUIRE(display.m_SelectionEnd == 7);
	REQUIRE(display.m_CompositionStart == 1);
	REQUIRE(display.m_CompositionEnd == 7);
	REQUIRE(display.m_Composing);
	REQUIRE(editor.m_Text == "A原文Z");
	composition.Clear();
	REQUIRE(composition.BuildDisplay(editor).m_Text == editor.m_Text);
	REQUIRE_FALSE(composition.BuildDisplay(editor).m_Composing);
}

TEST_CASE("Composition offsets clamp safely at UTF-8 character boundaries", "[text][ime]")
{
	Yukiri::Detail::TextEditor editor;
	Yukiri::Detail::TextComposition composition;
	composition.Update("你👩好", 1, std::numeric_limits<int>::max());
	auto display = composition.BuildDisplay(editor);
	REQUIRE(display.m_Caret == 3);
	REQUIRE(display.m_SelectionEnd == 10);
	composition.Update("你👩好", std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
	display = composition.BuildDisplay(editor);
	REQUIRE(display.m_Caret == display.m_Text.size());
	REQUIRE(display.m_SelectionEnd == display.m_Text.size());
	composition.Update("ni'hao", -1, -1);
	REQUIRE(composition.BuildDisplay(editor).m_Caret == 6);
	composition.Update("n\xFF\ni"s, 2, 1);
	display = composition.BuildDisplay(editor);
	REQUIRE(display.m_Text == "n\xEF\xBF\xBD i");
	REQUIRE(display.m_Caret == 4);
	REQUIRE(display.m_SelectionEnd == 5);
}

TEST_CASE("IME notifications and cancellation preserve editing history", "[text][ime]")
{
	Yukiri::Detail::WidgetTree tree;
	auto box = std::make_unique<Yukiri::TextBox>("original");
	auto* editor = box.get();
	tree.SetRoot(std::move(box));
	tree.SetFocus(editor);
	int changes = 0;
	auto connection = editor->m_TextChanged.Connect([&](std::string) { ++changes; });
	editor->SelectAll();
	tree.TextEditing("ni", 2, 0);
	tree.TextEditing("ni'hao", 6, 0);
	REQUIRE(changes == 0);
	tree.TextInput("你好");
	REQUIRE(editor->GetText() == "你好");
	REQUIRE(changes == 1);
	tree.KeyDown({Yukiri::Key::Z, {false, true, false}});
	REQUIRE(editor->GetText() == "original");
	REQUIRE(changes == 2);
	for (int operation = 0; operation < 4; ++operation)
	{
		tree.PointerDown({{1, 1}, Yukiri::PointerButton::Left});
		tree.SetFocus(editor);
		tree.TextEditing("cancel", 6, 0);
		if (operation == 0) editor->SetText(editor->GetText());
		if (operation == 1) tree.KeyDown({Yukiri::Key::Escape});
		if (operation == 2) { editor->SetReadOnly(true); editor->SetReadOnly(false); }
		if (operation == 3) { tree.LostFocus(); tree.SetFocus(editor); }
		tree.KeyDown({Yukiri::Key::Tab});
		REQUIRE(editor->IsFocusVisible());
		REQUIRE(editor->GetText() == "original");
		REQUIRE(changes == 2);
	}
}

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

TEST_CASE("Text shaping cache reuses results and separates DPI and font generations", "[text][cache]")
{
	auto created = Yukiri::Detail::TextEngine::Create();
	REQUIRE(created);
	auto& engine = **created;
	const auto width = engine.Measure("Cached 中文", 16, 1).m_Width;
	const auto hits = engine.GetShapeCacheHits();
	REQUIRE(engine.Measure("Cached 中文", 16, 1).m_Width == width);
	REQUIRE(engine.GetShapeCacheHits() == hits + 1);
	engine.Measure("Cached 中文", 16, 2);
	REQUIRE(engine.GetShapeCacheHits() == hits + 1);
	const auto bytes = engine.GetShapeCacheBytes();
	REQUIRE(bytes > 0);
	REQUIRE_FALSE(engine.LoadFont("missing-cache-font.ttf"));
	REQUIRE(engine.GetShapeCacheBytes() == bytes);
	bool reloaded = false;
	for (const auto& source : Yukiri::Detail::DiscoverFonts())
	{
		if (engine.LoadFont(source.m_Path, source.m_Index)) { reloaded = true; break; }
	}
	REQUIRE(reloaded);
	REQUIRE(engine.GetShapeCacheBytes() == 0);
	for (int i = 0; i < 3000; ++i) engine.Measure(std::to_string(i) + std::string(100, 'x'), 16);
	REQUIRE(engine.GetShapeCacheBytes() <= 8 * 1024 * 1024);
}
