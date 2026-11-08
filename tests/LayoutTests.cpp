#include <Yukiri/Layout/Grid.hpp>
#include <Yukiri/Layout/StackPanel.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("Stack layout respects margins padding alignment and hidden children", "[layout]")
{
	Yukiri::StackPanel panel(Yukiri::Orientation::Vertical, 5);
	panel.SetPadding(Yukiri::Insets(10));
	auto& first = panel.AddChild<Yukiri::Widget>();
	first.SetWidth(40);
	first.SetHeight(20);
	first.SetMargin(Yukiri::Insets(2));
	first.SetAlignment(Yukiri::Alignment::Center, Yukiri::Alignment::Stretch);
	auto& second = panel.AddChild<Yukiri::Widget>();
	second.SetWidth(60);
	second.SetHeight(30);
	panel.Measure({100, 200});
	REQUIRE(panel.GetDesiredSize().m_Width == Approx(80));
	REQUIRE(panel.GetDesiredSize().m_Height == Approx(79));
	panel.Arrange({0, 0, 100, 200});
	REQUIRE(first.GetBounds().m_X == Approx(30));
	REQUIRE(first.GetBounds().m_Y == Approx(12));
	REQUIRE(second.GetBounds().m_Y == Approx(39));
	second.SetVisible(false);
	panel.Measure({100, 200});
	REQUIRE(panel.GetDesiredSize().m_Height == Approx(44));
}

TEST_CASE("Grid allocates weighted tracks and validates spans", "[layout]")
{
	Yukiri::Grid grid;
	grid.SetColumns({Yukiri::GridLength::Pixels(50), Yukiri::GridLength::Star(), Yukiri::GridLength::Star(2)});
	grid.SetRows({Yukiri::GridLength::Auto()});
	grid.SetSpacing(5, 0);
	auto& child = grid.AddChild<Yukiri::Widget>();
	child.SetHeight(20);
	REQUIRE(grid.SetCell(child, 0, 1));
	REQUIRE_FALSE(grid.SetCell(child, 0, 2, 1, 2));
	grid.Measure({360, 100});
	grid.Arrange({0, 0, 360, 100});
	REQUIRE(child.GetBounds().m_X == Approx(55));
	REQUIRE(child.GetBounds().m_Width == Approx(100));
	REQUIRE(child.GetBounds().m_Height == Approx(20));
	REQUIRE(grid.SetCell(child, 0, 1, 1, 2));
	grid.Arrange({0, 0, 360, 100});
	REQUIRE(child.GetBounds().m_Width == Approx(305));
}

TEST_CASE("Empty rectangles and clipping use half-open boundaries", "[geometry]")
{
	const Yukiri::Rect bounds{10, 10, 20, 20};
	REQUIRE(bounds.Contains({10, 10}));
	REQUIRE_FALSE(bounds.Contains({30, 20}));
	REQUIRE(bounds.Intersect({40, 40, 10, 10}).Empty());
	REQUIRE(bounds.Intersect({20, 20, 20, 20}).m_Width == Approx(10));
}
