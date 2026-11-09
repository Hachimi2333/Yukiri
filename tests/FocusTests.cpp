#include "UI/WidgetTree.hpp"
#include <Yukiri/Controls/CheckBox.hpp>
#include <Yukiri/Controls/ToggleSwitch.hpp>
#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Controls/ScrollView.hpp>
#include <Yukiri/Controls/TabView.hpp>
#include <Yukiri/Layout/StackPanel.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
template<typename T>
class FocusProbe : public T
{
public:
	using T::T;
	using T::GetFocusBounds;
};
}

TEST_CASE("Focus visibility follows Tab navigation without changing logical focus", "[focus]")
{
	Yukiri::Detail::WidgetTree tree;
	auto button = std::make_unique<Yukiri::Button>("Only focus target");
	auto* control = button.get();
	tree.SetRoot(std::move(button));
	tree.Layout({200, 40});
	control->RequestFocus();
	REQUIRE(control->HasFocus());
	REQUIRE_FALSE(control->IsFocusVisible());
	tree.m_Dirty = false;
	tree.KeyDown({Yukiri::Key::Tab});
	REQUIRE(control->IsFocusVisible());
	REQUIRE(tree.m_Dirty);
	tree.PointerMove({{10, 10}});
	tree.Wheel({{10, 10}, Yukiri::PointerButton::None, {}, 0, -1});
	tree.TextInput("text");
	REQUIRE(control->IsFocusVisible());
	tree.m_Dirty = false;
	tree.PointerDown({{10, 10}, Yukiri::PointerButton::Left});
	REQUIRE(control->HasFocus());
	REQUIRE_FALSE(control->IsFocusVisible());
	REQUIRE(tree.m_Dirty);
	tree.PointerUp({{10, 10}, Yukiri::PointerButton::Left});
	tree.KeyDown({Yukiri::Key::Tab, {true, false, false}});
	REQUIRE(control->IsFocusVisible());
	tree.LostFocus();
	control->RequestFocus();
	REQUIRE_FALSE(control->IsFocusVisible());
}

TEST_CASE("Popup pointer input hides the ring before consuming the event", "[focus]")
{
	Yukiri::Detail::WidgetTree tree;
	auto combo = std::make_unique<Yukiri::ComboBox>(std::vector<std::string>{"One", "Two"});
	auto* control = combo.get();
	tree.SetRoot(std::move(combo));
	tree.Layout({220, 180});
	tree.KeyDown({Yukiri::Key::Tab});
	tree.KeyDown({Yukiri::Key::Down});
	REQUIRE(tree.HasPopup(control));
	REQUIRE(control->IsFocusVisible());
	tree.PointerDown({{12, 12}, Yukiri::PointerButton::Left});
	REQUIRE_FALSE(control->IsFocusVisible());
}

TEST_CASE("Focus display mode is isolated and survives safe focus removal", "[focus]")
{
	Yukiri::Detail::WidgetTree first;
	Yukiri::Detail::WidgetTree second;
	for (auto* tree : {&first, &second})
	{
		auto root = std::make_unique<Yukiri::Widget>();
		root->AddChild<Yukiri::Button>("A");
		root->AddChild<Yukiri::Button>("B");
		tree->SetRoot(std::move(root));
		tree->Layout({200, 40});
	}
	first.KeyDown({Yukiri::Key::Tab});
	second.m_Root->GetChildren()[0]->RequestFocus();
	REQUIRE(first.m_Focus->IsFocusVisible());
	REQUIRE_FALSE(second.m_Focus->IsFocusVisible());
	first.m_Focus->SetVisible(false);
	REQUIRE(first.m_Focus == nullptr);
	first.KeyDown({Yukiri::Key::Tab});
	REQUIRE(first.m_Focus->IsFocusVisible());
	first.m_Root->RemoveChild(*first.m_Focus);
	REQUIRE(first.m_Focus == nullptr);
}

TEST_CASE("Selection focus regions tightly surround padded icons and labels", "[focus]")
{
	Yukiri::Detail::WidgetTree tree;
	tree.m_MeasureText = [](std::string_view text, float, const Yukiri::Font&) { return Yukiri::Size{static_cast<float>(text.size()) * 8, 20}; };
	auto root = std::make_unique<Yukiri::StackPanel>();
	auto& check = root->AddChild<FocusProbe<Yukiri::CheckBox>>("Label");
	auto& radio = root->AddChild<FocusProbe<Yukiri::RadioButton>>("Label");
	auto& toggle = root->AddChild<FocusProbe<Yukiri::ToggleSwitch>>("Label");
	tree.SetRoot(std::move(root));
	tree.Layout({500, 200});
	for (const auto rectangle : {check.GetFocusBounds(), radio.GetFocusBounds(), toggle.GetFocusBounds()})
	{
		REQUIRE(rectangle.m_X == 2);
		REQUIRE(rectangle.m_Y >= 2);
		REQUIRE(rectangle.Right() < 110);
		REQUIRE(rectangle.Bottom() <= 32);
		REQUIRE(rectangle.m_X + 4 <= 6);
	}
	REQUIRE(check.GetFocusBounds().Right() == 78);
	REQUIRE(toggle.GetFocusBounds().Right() == 98);
	tree.Layout({30, 200});
	REQUIRE(toggle.GetFocusBounds().Right() <= 28);
}

TEST_CASE("Tab focus and scrolling target only the selected header", "[focus]")
{
	Yukiri::Detail::WidgetTree tree;
	auto scroll = std::make_unique<Yukiri::ScrollView>();
	auto* view = scroll.get();
	view->SetFocusable(false);
	auto tabs = std::make_unique<FocusProbe<Yukiri::TabView>>();
	auto* control = tabs.get();
	for (int i = 0; i < 2; ++i)
	{
		auto content = std::make_unique<Yukiri::Widget>();
		content->SetHeight(1000);
		tabs->AddTab("Page", std::move(content));
	}
	view->SetContent(std::move(tabs));
	tree.SetRoot(std::move(scroll));
	tree.Layout({300, 150});
	tree.KeyDown({Yukiri::Key::Tab});
	REQUIRE(tree.m_Focus == control);
	REQUIRE(view->GetOffset().m_Y == 0);
	const auto before = control->GetFocusBounds();
	REQUIRE(before.Bottom() < 35);
	tree.KeyDown({Yukiri::Key::Right});
	REQUIRE(control->GetFocusBounds().m_X > before.m_X);
	REQUIRE(control->GetFocusBounds().Bottom() < 35);
	REQUIRE(view->GetOffset().m_Y == 0);
	tree.PointerDown({{10, 10}, Yukiri::PointerButton::Left});
	REQUIRE_FALSE(control->IsFocusVisible());
	REQUIRE(view->GetOffset().m_Y == 0);
}
