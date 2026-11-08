#include "UI/WidgetTree.hpp"
#include "Controls/PopupLayout.hpp"
#include <Yukiri/Controls/Button.hpp>
#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Controls/Slider.hpp>
#include <Yukiri/Controls/TabView.hpp>
#include <Yukiri/Controls/ScrollView.hpp>
#include <Yukiri/Layout/StackPanel.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Button activation requires release inside and honors disabled focus", "[controls]")
{
	Yukiri::Detail::WidgetTree tree;
	auto button = std::make_unique<Yukiri::Button>("Action");
	auto* control = button.get();
	int activations = 0;
	auto connection = control->m_Clicked.Connect([&] { ++activations; });
	tree.SetRoot(std::move(button));
	tree.Layout({100, 40});
	tree.PointerDown({{10, 10}, Yukiri::PointerButton::Left});
	tree.PointerUp({{200, 10}, Yukiri::PointerButton::Left});
	REQUIRE(activations == 0);
	tree.PointerDown({{10, 10}, Yukiri::PointerButton::Left});
	tree.PointerUp({{10, 10}, Yukiri::PointerButton::Left});
	REQUIRE(activations == 1);
	tree.KeyDown({Yukiri::Key::Enter});
	REQUIRE(activations == 2);
	control->SetEnabled(false);
	tree.KeyDown({Yukiri::Key::Enter});
	REQUIRE(activations == 2);
	REQUIRE(tree.m_Focus == nullptr);
}

TEST_CASE("Radio groups are exclusive among sibling controls", "[controls]")
{
	Yukiri::Widget root;
	auto& first = root.AddChild<Yukiri::RadioButton>("First", "Group");
	auto& second = root.AddChild<Yukiri::RadioButton>("Second", "Group");
	auto& other = root.AddChild<Yukiri::RadioButton>("Other", "OtherGroup");
	first.SetChecked(true);
	other.SetChecked(true);
	second.SetChecked(true);
	REQUIRE_FALSE(first.IsChecked());
	REQUIRE(second.IsChecked());
	REQUIRE(other.IsChecked());
}

TEST_CASE("Combo popup selection uses keyboard and closes without extra activation", "[controls]")
{
	Yukiri::Detail::WidgetTree tree;
	auto combo = std::make_unique<Yukiri::ComboBox>(std::vector<std::string>{"One", "Two", "Three"});
	auto* control = combo.get();
	tree.SetRoot(std::move(combo));
	tree.Layout({200, 40});
	tree.SetFocus(control);
	tree.KeyDown({Yukiri::Key::Down});
	REQUIRE(tree.HasPopup(control));
	tree.KeyDown({Yukiri::Key::Down});
	tree.KeyDown({Yukiri::Key::Enter});
	REQUIRE(control->GetSelectedIndex() == 1);
	REQUIRE_FALSE(tree.HasPopup(control));
	REQUIRE_FALSE(control->SetSelectedIndex(7));
}

TEST_CASE("Scroll offsets clamp and focused descendants become visible", "[controls]")
{
	Yukiri::Detail::WidgetTree tree;
	auto scroll = std::make_unique<Yukiri::ScrollView>();
	auto* view = scroll.get();
	auto content = std::make_unique<Yukiri::StackPanel>(Yukiri::Orientation::Vertical, 0.0f);
	Yukiri::Button* last = nullptr;
	for (int i = 0; i < 10; ++i) { last = &content->AddChild<Yukiri::Button>("Row"); last->SetHeight(30); }
	view->SetContent(std::move(content));
	tree.SetRoot(std::move(scroll));
	tree.Layout({200, 100});
	tree.SetFocus(last);
	tree.Layout({200, 100});
	REQUIRE(view->GetOffset().m_Y == Catch::Approx(200));
	REQUIRE(last->GetBounds().Bottom() <= 100);
	view->SetOffset({0, 9999});
	REQUIRE(view->GetOffset().m_Y == Catch::Approx(200));
	REQUIRE(view->HitTest({195, 50}) == view);
}

TEST_CASE("Tab selection hides inactive focus candidates and validates indices", "[controls]")
{
	Yukiri::Detail::WidgetTree tree;
	auto tabs = std::make_unique<Yukiri::TabView>();
	auto* control = tabs.get();
	auto& first = tabs->AddTab("First", std::make_unique<Yukiri::Button>("A"));
	auto& second = tabs->AddTab("Second", std::make_unique<Yukiri::Button>("B"));
	tree.SetRoot(std::move(tabs));
	tree.Layout({300, 100});
	REQUIRE(first.IsVisible());
	REQUIRE_FALSE(second.IsVisible());
	REQUIRE(control->SetSelectedIndex(1));
	REQUIRE_FALSE(first.IsVisible());
	REQUIRE(second.IsVisible());
	REQUIRE_FALSE(control->SetSelectedIndex(2));
}

TEST_CASE("Slider clamps values and rejects invalid ranges", "[controls]")
{
	Yukiri::Slider slider;
	REQUIRE(slider.SetRange(-10, 10));
	slider.SetValue(99);
	REQUIRE(slider.GetValue() == 10);
	REQUIRE_FALSE(slider.SetRange(10, 10));
	Yukiri::KeyEvent left{Yukiri::Key::Left};
	slider.OnKeyDown(left);
	REQUIRE(slider.GetValue() == Catch::Approx(9.8));
}

TEST_CASE("Popup placement stays inside the content viewport", "[controls]")
{
	const Yukiri::Rect viewport{0, 40, 330, 200};
	const auto popup = Yukiri::Detail::PlacePopup({280, 210, 160, 34}, viewport, 20, 32);
	REQUIRE(popup.m_VisibleRows < 7);
	REQUIRE(popup.m_Bounds.m_X >= viewport.m_X);
	REQUIRE(popup.m_Bounds.m_Y >= viewport.m_Y);
	REQUIRE(popup.m_Bounds.Right() <= viewport.Right());
	REQUIRE(popup.m_Bounds.Bottom() <= viewport.Bottom());
	REQUIRE(Yukiri::Detail::PlacePopup({}, {}, 4, 32).m_VisibleRows == 0);
}
