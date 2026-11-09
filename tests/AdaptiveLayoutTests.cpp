#include "UI/WidgetTree.hpp"
#include "Text/TextLayout.hpp"
#include "Text/Unicode.hpp"
#include <Yukiri/Controls/Label.hpp>
#include <Yukiri/Controls/ScrollView.hpp>
#include <Yukiri/Layout/Border.hpp>
#include <Yukiri/Layout/Grid.hpp>
#include <Yukiri/Layout/SplitPanel.hpp>
#include <Yukiri/Layout/WrapPanel.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using Catch::Approx;

namespace
{
Yukiri::Size Monospace(std::string_view text)
{
	return {static_cast<float>(Yukiri::Detail::GraphemeBoundaries(text).size() - 1) * 10, 20};
}
}

TEST_CASE("Wrapped layout preserves hard breaks and splits long words at grapheme boundaries", "[text][layout]")
{
	const std::string text = "abcdefghij\n\n中e\xCC\x81👩‍💻";
	const auto lines = Yukiri::Detail::LayoutText(text, 30, true, Monospace);
	REQUIRE(lines.size() == 6);
	REQUIRE(text.substr(lines[0].m_Start, lines[0].m_End - lines[0].m_Start) == "abc");
	REQUIRE(lines[4].m_Start == lines[4].m_End);
	REQUIRE(text.substr(lines.back().m_Start) == "中e\xCC\x81👩‍💻");
	for (const auto& line : lines) REQUIRE(line.m_Width <= 30);
	REQUIRE(Yukiri::Detail::FitText("Ae\xCC\x81👩‍💻BCD", 30, Monospace) == "Ae\xCC\x81…");
	REQUIRE(Yukiri::Detail::FitText("Text", 5, Monospace).empty());
	const auto zero = Yukiri::Detail::LayoutText("👩‍💻A", 0, true, Monospace);
	REQUIRE(zero.size() == 2);
}

TEST_CASE("Grid measures wrapping text against its allocated column width", "[layout]")
{
	Yukiri::Detail::WidgetTree tree;
	tree.m_MeasureText = [](std::string_view text, float, const Yukiri::Font&) { return Monospace(text); };
	auto grid = std::make_unique<Yukiri::Grid>();
	grid->SetColumns({Yukiri::GridLength::Pixels(50), Yukiri::GridLength::Star()});
	grid->SetRows({Yukiri::GridLength::Auto()});
	grid->SetSpacing(0, 0);
	auto& label = grid->AddChild<Yukiri::Label>("abcdefghijklmnopqrst");
	label.SetWrap(true);
	REQUIRE(grid->SetCell(label, 0, 1));
	tree.SetRoot(std::move(grid));
	tree.Layout({120, 200});
	REQUIRE(label.GetBounds().m_Width == Approx(70));
	REQUIRE(label.GetDesiredSize().m_Height == Approx(60));
	REQUIRE(label.GetBounds().m_Height == Approx(60));
}

TEST_CASE("Wrap panels reflow visible children and border padding contributes to constraints", "[layout]")
{
	Yukiri::WrapPanel panel;
	panel.SetSpacing(5, 7);
	for (const auto height : {10, 20, 15})
	{
		auto& child = panel.AddChild<Yukiri::Widget>();
		child.SetWidth(40);
		child.SetHeight(static_cast<float>(height));
	}
	panel.Measure({90, 100});
	panel.Arrange({0, 0, 90, 100});
	REQUIRE(panel.GetDesiredSize().m_Width == Approx(85));
	REQUIRE(panel.GetDesiredSize().m_Height == Approx(42));
	REQUIRE(panel.GetChildren()[2]->GetBounds().m_Y == Approx(27));
	panel.GetChildren()[0]->SetVisible(false);
	panel.Measure({90, 100});
	REQUIRE(panel.GetDesiredSize().m_Height == Approx(20));
	Yukiri::Border border;
	auto content = std::make_unique<Yukiri::Widget>();
	content->SetWidth(40);
	content->SetHeight(20);
	border.SetContent(std::move(content));
	border.Measure({100, 80});
	border.Arrange({0, 0, 100, 80});
	REQUIRE(border.GetDesiredSize().m_Width == Approx(64));
	REQUIRE(border.GetChildren()[0]->GetBounds().m_X == Approx(12));
}

TEST_CASE("Split panels obey pane minimums and cancel pointer resizing safely", "[layout][input]")
{
	Yukiri::Detail::WidgetTree tree;
	auto panel = std::make_unique<Yukiri::SplitPanel>();
	auto* split = panel.get();
	panel->SetContent(std::make_unique<Yukiri::Widget>(), std::make_unique<Yukiri::Widget>());
	panel->SetMinimumSizes(40, 60);
	panel->SetRatio(0.1f);
	tree.SetRoot(std::move(panel));
	tree.Layout({306, 100});
	REQUIRE(split->GetChildren()[0]->GetBounds().m_Width == Approx(40));
	tree.SetFocus(split);
	tree.KeyDown({Yukiri::Key::Right});
	tree.Layout({306, 100});
	REQUIRE(split->GetChildren()[0]->GetBounds().m_Width == Approx(50));
	tree.PointerDown({{52, 10}, Yukiri::PointerButton::Left});
	tree.PointerMove({{202, 10}});
	tree.Layout({306, 100});
	REQUIRE(split->GetChildren()[0]->GetBounds().m_Width == Approx(200));
	tree.CancelPointer();
	tree.PointerMove({{12, 10}});
	REQUIRE(split->GetRatio() == Approx(200.0f / 300));
	tree.Layout({50, 100});
	const auto first = split->GetChildren()[0]->GetBounds();
	const auto second = split->GetChildren()[1]->GetBounds();
	REQUIRE(first.m_Width >= 0);
	REQUIRE(second.Right() == Approx(50));
}

TEST_CASE("Split divider hit testing follows scrolling without another measure pass", "[layout][scroll]")
{
	Yukiri::Detail::WidgetTree tree;
	auto scroll = std::make_unique<Yukiri::ScrollView>();
	auto* viewport = scroll.get();
	auto content = std::make_unique<Yukiri::StackPanel>();
	content->AddChild<Yukiri::Widget>().SetHeight(100);
	auto& split = content->AddChild<Yukiri::SplitPanel>();
	split.SetHeight(100);
	split.SetContent(std::make_unique<Yukiri::Widget>(), std::make_unique<Yukiri::Widget>());
	content->AddChild<Yukiri::Widget>().SetHeight(500);
	scroll->SetContent(std::move(content));
	tree.SetRoot(std::move(scroll));
	tree.Layout({306, 200});
	viewport->SetOffset({0, 100});
	const auto first = split.GetChildren()[0]->GetBounds();
	tree.PointerDown({{first.Right() + 2, first.m_Y + 10}, Yukiri::PointerButton::Left});
	REQUIRE(tree.m_Capture == &split);
	tree.CancelPointer();
}
