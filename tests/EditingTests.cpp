#include <Yukiri/Controls/TextArea.hpp>
#include <Yukiri/Controls/NumberBox.hpp>
#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Controls/Slider.hpp>
#include <Yukiri/Controls/ProgressBar.hpp>
#include <Yukiri/Layout/StackPanel.hpp>
#include "UI/WidgetTree.hpp"
#include "Text/TextEditor.hpp"
#include "Text/TextVisual.hpp"
#include "Text/DocumentLayout.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
Yukiri::Size Measure(std::string_view text) { return {static_cast<float>(Yukiri::Detail::GraphemeBoundaries(text).size() - 1) * 8, 20}; }
auto Carets(std::string_view text)
{
	std::vector<std::pair<std::size_t, float>> result;
	float x = 0;
	for (const auto boundary : Yukiri::Detail::GraphemeBoundaries(text)) { result.emplace_back(boundary, x); x += 8; }
	return result;
}
void Metrics(Yukiri::Detail::WidgetTree& tree) { tree.m_MeasureText = [](auto text, auto, const auto&) { return Measure(text); }; }
}
TEST_CASE("Multiline editor normalizes newlines and groups contiguous typing and deletion", "[editing]")
{
	Yukiri::Detail::TextEditor editor(false);
	editor.SetText("a\r\nb\rc\t");
	REQUIRE(editor.m_Text == "a\nb\nc\t");
	editor.SetText("");
	editor.Insert("你", true); editor.Insert("好", true);
	editor.Undo();
	REQUIRE(editor.m_Text.empty());
	editor.Redo();
	REQUIRE(editor.m_Text == "你好");
	editor.Backspace(); editor.Backspace();
	REQUIRE(editor.m_Text.empty());
	editor.Undo();
	REQUIRE(editor.m_Text == "你好");
	editor.MoveTo(0, false);
	editor.Delete(); editor.Delete();
	editor.Undo();
	REQUIRE(editor.m_Text == "你好");
}
TEST_CASE("Literal search and replace preserve graphemes and form one undoable transaction", "[editing][search]")
{
	Yukiri::Detail::TextEditor editor(false);
	editor.SetText("Äpfel äpfel\n中文 中文 e\xCC\x81");
	REQUIRE(editor.Find("ÄPFEL", 0, false)->first == 0);
	REQUIRE(editor.Find("中文", editor.m_Text.size(), true, true)->first > 10);
	REQUIRE_FALSE(editor.Find("\xCC\x81", 0, true));
	const auto original = editor.m_Text;
	REQUIRE(editor.ReplaceAll("äpfel", "fruit", false) == 2);
	REQUIRE(editor.m_Text.starts_with("fruit fruit\n"));
	editor.Undo();
	REQUIRE(editor.m_Text == original);
	REQUIRE(editor.ReplaceAll("", "x", true) == 0);
	editor.SetText("aaaa");
	REQUIRE(editor.ReplaceAll("aa", "b", true) == 2);
	REQUIRE(editor.m_Text == "bb");
}
TEST_CASE("Paragraph layout reuses unaffected metrics and limits visible line enumeration", "[editing][layout]")
{
	Yukiri::Detail::DocumentLayout layout;
	std::vector<std::string> measured;
	auto measure = [&](auto text) { measured.emplace_back(text); return Measure(text); };
	layout.Update("one\ntwo\nthree", 200, true, 20, 1, measure, Carets);
	measured.clear();
	layout.Update("one\nchanged\nthree", 200, true, 20, 1, measure, Carets);
	REQUIRE_FALSE(measured.empty());
	REQUIRE(std::ranges::none_of(measured, [](const auto& text) { return text == "one" || text == "three"; }));
	measured.clear();
	layout.Update("one\nchanged\nthree", 200, true, 20, 1, measure, Carets);
	REQUIRE(measured.empty());
	REQUIRE(layout.Hit(1, 20) == 7);
	REQUIRE(layout.CaretX(1, 7) == 24);
	std::string text;
	for (int index = 0; index < 10000; ++index) text += "row\n";
	layout.Update(text, 200, true, 20, 1, Measure, Carets);
	const auto visible = layout.Visible(100000, 100);
	REQUIRE(visible.first == 5000);
	REQUIRE(visible.second - visible.first == 5);
}
TEST_CASE("Multiline navigation preserves horizontal intent and selects across visual lines", "[editing][input]")
{
	Yukiri::Detail::WidgetTree tree;
	Metrics(tree);
	auto text = std::make_unique<Yukiri::TextArea>("abc\ndefgh\n中e\xCC\x81");
	auto* editor = text.get();
	tree.SetRoot(std::move(text));
	tree.Layout({240, 140});
	tree.SetFocus(editor);
	editor->SetSelection(2, 2);
	tree.KeyDown({Yukiri::Key::Down});
	REQUIRE(editor->GetSelection().second == 6);
	tree.KeyDown({Yukiri::Key::Down});
	REQUIRE(editor->GetSelection().second == editor->GetText().size());
	tree.KeyDown({Yukiri::Key::Up});
	REQUIRE(editor->GetSelection().second == 6);
	tree.KeyDown({Yukiri::Key::Home, {true}});
	REQUIRE(editor->GetSelection() == std::pair<std::size_t, std::size_t>{6, 4});
	tree.TextInput("X\nY");
	REQUIRE(editor->GetText().starts_with("abc\nX\nYfgh"));
	editor->Undo();
	REQUIRE(editor->GetText() == "abc\ndefgh\n中e\xCC\x81");
}
TEST_CASE("Multiline composition positions the caret from display paragraphs without committing preedit", "[editing][ime]")
{
	Yukiri::Detail::WidgetTree tree;
	Metrics(tree);
	auto text = std::make_unique<Yukiri::TextArea>("A\n原Z");
	auto* editor = text.get();
	tree.SetRoot(std::move(text));
	tree.Layout({240, 140});
	tree.SetFocus(editor);
	editor->SetSelection(2, 5);
	int changed = 0;
	auto connection = editor->m_TextChanged.Connect([&](auto) { ++changed; });
	editor->SetText("A\r\n原Z");
	REQUIRE(changed == 0);
	tree.TextEditing("ni\nhao", 4, 0);
	REQUIRE(editor->GetText() == "A\n原Z");
	REQUIRE(editor->GetCaretBounds().m_Y == Catch::Approx(49.2f));
	REQUIRE(editor->GetCaretBounds().m_X == 18);
	REQUIRE(changed == 0);
	tree.TextInput("你\n好");
	REQUIRE(editor->GetText() == "A\n你\n好Z");
	REQUIRE(changed == 1);
	editor->Undo();
	REQUIRE(editor->GetText() == "A\n原Z");
}
TEST_CASE("Password display masks graphemes and blocks clipboard and accessible plaintext", "[editing][password]")
{
	Yukiri::Detail::TextEditor model;
	model.SetText("中e\xCC\x81");
	Yukiri::Detail::TextComposition composition;
	const auto visual = Yukiri::Detail::BuildTextVisual(composition.BuildDisplay(model), true);
	REQUIRE(visual.m_Display.m_Text == "••");
	REQUIRE(visual.ToSource(3) == 3);
	Yukiri::TextBox box("secret");
	box.SetPassword(true);
	box.SelectAll();
	REQUIRE(box.GetAccessibleText().empty());
	REQUIRE_FALSE(box.Copy());
	REQUIRE_FALSE(box.Cut());
	REQUIRE(box.GetText() == "secret");
	box.SetReadOnly(true);
	box.OnTextInput("ignored");
	REQUIRE(box.GetText() == "secret");
	box.SetPassword(false);
	box.SetSelection(0, 0);
	REQUIRE(box.Copy());
}

TEST_CASE("Text scrolling reuses metrics, reverses smoothly, and clamps after document shrink", "[editing][scroll]")
{
	Yukiri::Detail::WidgetTree tree;
	int measurements = 0;
	tree.m_MeasureText = [&](auto text, auto, const auto&) { ++measurements; return Measure(text); };
	std::string document;
	for (int index = 0; index < 100; ++index) document += "a row\n";
	auto text = std::make_unique<Yukiri::TextArea>(document);
	auto* editor = text.get();
	tree.SetRoot(std::move(text)); tree.Layout({240, 140});
	const auto baseline = measurements;
	const auto now = std::chrono::steady_clock::now();
	tree.Wheel({{30, 30}, Yukiri::PointerButton::None, {}, 0, -3});
	tree.Tick(now + std::chrono::milliseconds(40));
	REQUIRE(editor->GetOffset().m_Y > 0);
	REQUIRE(editor->GetOffset().m_Y < 120);
	tree.Wheel({{30, 30}, Yukiri::PointerButton::None, {}, 0, 3});
	tree.Tick(now + std::chrono::seconds(1));
	REQUIRE(editor->GetOffset().m_Y == 0);
	REQUIRE(measurements == baseline);
	editor->SetOffset({0, 1000});
	editor->SetText("short");
	const auto caret = editor->GetCaretBounds(); (void)caret;
	REQUIRE(editor->GetOffset().m_Y == 0);
}
TEST_CASE("Numeric input rejects invalid commits and supports format, range, and keyboard steps", "[number-box]")
{
	Yukiri::Detail::WidgetTree tree;
	auto number = std::make_unique<Yukiri::NumberBox>(2);
	auto* control = number.get();
	REQUIRE(control->SetRange(0, 10));
	REQUIRE(control->SetStep(0.25));
	REQUIRE(control->SetFormat(Yukiri::NumberBox::Format::Fixed, 2));
	tree.SetRoot(std::move(number));
	tree.Layout({220, 40});
	control->RequestFocus();
	REQUIRE(control->GetEditor().HasFocus());
	control->GetEditor().SetText("invalid");
	REQUIRE_FALSE(control->Commit());
	REQUIRE_FALSE(control->IsValid());
	REQUIRE(control->GetValue() == 2);
	tree.KeyDown({Yukiri::Key::Escape});
	REQUIRE(control->IsValid());
	REQUIRE(control->GetEditor().GetText() == "2.00");
	tree.KeyDown({Yukiri::Key::Up});
	REQUIRE(control->GetValue() == 2.25);
	control->GetEditor().SetText("11");
	REQUIRE_FALSE(control->Commit());
	control->SetValue(99);
	REQUIRE(control->GetValue() == 10);
	REQUIRE_FALSE(control->SetStep(0));
	control->SetReadOnly(true);
	tree.KeyDown({Yukiri::Key::Down});
	REQUIRE(control->GetValue() == 10);
}
TEST_CASE("Radio groups, closed combos, slider steps, and indeterminate motion follow keyboard conventions", "[controls][keyboard]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::StackPanel>();
	auto& first = root->AddChild<Yukiri::RadioButton>("First");
	auto& disabled = root->AddChild<Yukiri::RadioButton>("Disabled"); disabled.SetEnabled(false);
	auto& second = root->AddChild<Yukiri::RadioButton>("Second");
	auto& combo = root->AddChild<Yukiri::ComboBox>(std::vector<std::string>{"One", "Two", "Three"});
	auto& slider = root->AddChild<Yukiri::Slider>();
	REQUIRE(slider.SetRange(0, 10)); REQUIRE(slider.SetStep(0.5));
	tree.SetRoot(std::move(root)); tree.Layout({240, 300});
	tree.KeyDown({Yukiri::Key::Tab}); REQUIRE(first.HasFocus());
	tree.KeyDown({Yukiri::Key::Right}); REQUIRE(second.HasFocus()); REQUIRE(second.IsChecked());
	tree.KeyDown({Yukiri::Key::Tab}); REQUIRE(combo.HasFocus());
	tree.KeyDown({Yukiri::Key::Down}); REQUIRE(combo.GetSelectedIndex() == 1); REQUIRE_FALSE(combo.IsExpanded());
	tree.KeyDown({Yukiri::Key::End}); REQUIRE(combo.GetSelectedIndex() == 2);
	tree.KeyDown({Yukiri::Key::Tab}); REQUIRE(slider.HasFocus());
	tree.KeyDown({Yukiri::Key::Right}); REQUIRE(slider.GetValue() == 0.5f);
	tree.KeyDown({Yukiri::Key::End}); REQUIRE(slider.GetValue() == 10);
	Yukiri::ProgressBar progress;
	progress.SetIndeterminate(true); REQUIRE(progress.IsIndeterminate());
}
