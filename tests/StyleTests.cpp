#include "UI/WidgetTree.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

class StyledProbe : public Yukiri::Widget
{
public:
	float Hover() const { return GetHoverAmount(); }
};

TEST_CASE("Theme overrides inherit through the widget tree", "[style]")
{
	Yukiri::Detail::WidgetTree tree;
	tree.m_Theme = Yukiri::Theme::Dark();
	auto root = std::make_unique<Yukiri::Widget>();
	auto& child = root->AddChild<Yukiri::Widget>();
	tree.SetRoot(std::move(root));
	REQUIRE(child.GetTheme().m_Background.m_R == tree.m_Theme.m_Background.m_R);
	tree.m_Root->SetThemeOverride(Yukiri::Theme::Light());
	REQUIRE(child.GetTheme().m_Background.m_R == Yukiri::Theme::Light().m_Background.m_R);
	Yukiri::WidgetStyle style;
	style.m_FontSize = 20;
	child.SetStyle(style);
	REQUIRE(child.GetFontSize() == 20);
}

TEST_CASE("Hover transitions finish and stop scheduling animation frames", "[style]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<StyledProbe>();
	auto* probe = root.get();
	tree.SetRoot(std::move(root));
	tree.Layout({100, 100});
	tree.PointerMove({{10, 10}});
	tree.Tick();
	REQUIRE(tree.m_Animating);
	tree.Tick(std::chrono::steady_clock::now() + std::chrono::seconds(1));
	REQUIRE(probe->Hover() == Catch::Approx(1));
	REQUIRE_FALSE(tree.m_Animating);
}
