#include <Yukiri/Yukiri.hpp>
#include "UI/WidgetTree.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

namespace
{
class SwitchProbe : public Yukiri::ToggleSwitch
{
public:
	using ToggleSwitch::ToggleSwitch;
	float Visual() const { return m_CheckVisual.GetValue(); }
};
class MeasureProbe : public Yukiri::StackPanel
{
public:
	MeasureProbe() : StackPanel(Yukiri::Orientation::Vertical, 0) {}
	int m_Measures = 0;
protected:
	Yukiri::Size OnMeasure(Yukiri::Size available) override { ++m_Measures; return StackPanel::OnMeasure(available); }
};
}

TEST_CASE("Checked motion changes the model immediately and reverses from its visible state", "[controls][motion]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<SwitchProbe>("Switch", true);
	auto* control = root.get();
	tree.SetRoot(std::move(root));
	tree.Layout({200, 40});
	REQUIRE(control->Visual() == 1);
	int notifications = 0;
	auto connection = control->m_CheckedChanged.Connect([&](bool) { ++notifications; });
	const auto now = std::chrono::steady_clock::now();
	control->SetChecked(false);
	REQUIRE_FALSE(control->IsChecked());
	REQUIRE(control->Visual() == 1);
	tree.Tick(now + 80ms);
	const auto middle = control->Visual();
	REQUIRE(middle > 0);
	REQUIRE(middle < 1);
	control->SetChecked(true);
	REQUIRE(control->Visual() == middle);
	tree.Tick(now + 100ms);
	REQUIRE(control->Visual() > middle);
	tree.Tick(now + 1s);
	REQUIRE(control->Visual() == 1);
	REQUIRE(notifications == 2);
	REQUIRE_FALSE(tree.m_Animating);
	tree.m_Theme.m_Motion.m_Enabled = false;
	control->SetChecked(false);
	REQUIRE(control->Visual() == 0);
}

TEST_CASE("Smooth scrolling accumulates and reverses without measuring content again", "[scroll][motion]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::ScrollView>();
	auto* view = root.get();
	auto content = std::make_unique<MeasureProbe>();
	auto* measured = content.get();
	std::vector<Yukiri::Widget*> rows;
	for (int i = 0; i < 10; ++i) { auto& row = content->AddChild<Yukiri::Button>("Row"); row.SetHeight(30); rows.push_back(&row); }
	view->SetContent(std::move(content));
	tree.SetRoot(std::move(root));
	tree.Layout({200, 100});
	const int measures = measured->m_Measures;
	const auto now = std::chrono::steady_clock::now();
	Yukiri::PointerEvent wheel{{10, 10}, Yukiri::PointerButton::None, {}, 0, -1};
	view->OnWheel(wheel);
	REQUIRE(wheel.m_Handled);
	REQUIRE(view->GetOffset().m_Y == 0);
	tree.Tick(now + 35ms);
	const auto first = view->GetOffset().m_Y;
	REQUIRE(first > 10);
	REQUIRE(first < 30);
	REQUIRE(rows[0]->GetBounds().m_Y == Catch::Approx(-first));
	REQUIRE(view->HitTest({10, 10}) == rows[1]);
	view->OnWheel(wheel);
	tree.Tick(now + 70ms);
	const auto second = view->GetOffset().m_Y;
	REQUIRE(second > 40);
	wheel.m_WheelY = 1;
	view->OnWheel(wheel);
	REQUIRE(view->GetOffset().m_Y == second);
	tree.Tick(now + 90ms);
	REQUIRE(view->GetOffset().m_Y < second);
	tree.Tick(now + 1s);
	REQUIRE(view->GetOffset().m_Y == Catch::Approx(40));
	REQUIRE(measured->m_Measures == measures);
	view->SetOffset({0, 100});
	tree.Tick(now + 2s);
	REQUIRE(view->GetOffset().m_Y == 100);
	view->SetSmoothScrolling(false);
	wheel.m_WheelY = -0.5f;
	view->OnWheel(wheel);
	REQUIRE(view->GetOffset().m_Y == 120);
	view->SetSmoothScrolling(true);
	Yukiri::KeyEvent home{Yukiri::Key::Home};
	view->OnKeyDown(home);
	REQUIRE(home.m_Handled);
	REQUIRE(view->GetOffset().m_Y == 120);
	tree.Tick(now + 3s);
	REQUIRE(view->GetOffset().m_Y == 0);
	Yukiri::KeyEvent end{Yukiri::Key::End};
	view->OnKeyDown(end);
	tree.Tick(now + 4s);
	REQUIRE(view->GetOffset().m_Y == 200);
	REQUIRE(measured->m_Measures == measures);
}

TEST_CASE("Scrollbar dragging cancels smooth motion and follows the pointer directly", "[scroll][motion]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::ScrollView>();
	auto* view = root.get();
	auto content = std::make_unique<Yukiri::Widget>();
	content->SetHeight(1000);
	view->SetContent(std::move(content));
	tree.SetRoot(std::move(root));
	tree.Layout({200, 100});
	Yukiri::PointerEvent wheel{{10, 10}, Yukiri::PointerButton::None, {}, 0, -2};
	view->OnWheel(wheel);
	Yukiri::PointerEvent down{{195, 5}, Yukiri::PointerButton::Left};
	view->OnPointerDown(down);
	const auto now = std::chrono::steady_clock::now();
	tree.Tick(now + 1s);
	REQUIRE(view->GetOffset().m_Y == 0);
	Yukiri::PointerEvent move{{195, 20}};
	view->OnPointerMove(move);
	REQUIRE(view->GetOffset().m_Y > 100);
	view->OnPointerUp(down);
	REQUIRE(tree.m_Capture == nullptr);
}

TEST_CASE("Nested wheel input bubbles when the inner target reaches its edge", "[scroll][motion]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::ScrollView>();
	auto* outer = root.get();
	auto content = std::make_unique<Yukiri::StackPanel>();
	auto& inner = content->AddChild<Yukiri::ScrollView>();
	inner.SetHeight(60);
	auto innerContent = std::make_unique<Yukiri::Widget>();
	innerContent->SetHeight(300);
	inner.SetContent(std::move(innerContent));
	content->AddChild<Yukiri::Widget>().SetHeight(400);
	outer->SetContent(std::move(content));
	tree.SetRoot(std::move(root));
	tree.Layout({200, 200});
	inner.SetOffset({0, 1000});
	const auto end = inner.GetOffset().m_Y;
	tree.Wheel({{10, 10}, Yukiri::PointerButton::None, {}, 0, -1});
	tree.Tick(std::chrono::steady_clock::now() + 1s);
	REQUIRE(inner.GetOffset().m_Y == end);
	REQUIRE(outer->GetOffset().m_Y == Catch::Approx(40));
}

TEST_CASE("Content shrinking clamps both current and pending scroll positions", "[scroll][motion]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::ScrollView>();
	auto* view = root.get();
	auto content = std::make_unique<Yukiri::Widget>();
	auto* resized = content.get();
	content->SetHeight(1000);
	view->SetContent(std::move(content));
	tree.SetRoot(std::move(root));
	tree.Layout({200, 100});
	Yukiri::PointerEvent wheel{{10, 10}, Yukiri::PointerButton::None, {}, 0, -20};
	view->OnWheel(wheel);
	tree.Tick(std::chrono::steady_clock::now() + 40ms);
	REQUIRE(view->GetOffset().m_Y > 0);
	resized->SetHeight(20);
	tree.Layout({200, 100});
	tree.Tick(std::chrono::steady_clock::now() + 1s);
	REQUIRE(view->GetOffset().m_Y == 0);
	REQUIRE_FALSE(tree.m_Animating);
}

TEST_CASE("Popup closing relinquishes input immediately and owner removal cancels its visual", "[popup][motion]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::ComboBox>(std::vector<std::string>{"One", "Two"});
	auto* combo = root.get();
	tree.SetRoot(std::move(root));
	tree.Layout({200, 40});
	tree.SetFocus(combo);
	tree.KeyDown({Yukiri::Key::Down});
	tree.Tick(std::chrono::steady_clock::now() + 40ms);
	REQUIRE(tree.HasPopup(combo));
	tree.ClosePopup();
	REQUIRE_FALSE(tree.HasPopup(combo));
	REQUIRE(tree.OwnsPopup(combo));
	tree.KeyDown({Yukiri::Key::Down});
	REQUIRE(tree.HasPopup(combo));
	tree.SetRoot(nullptr);
	REQUIRE_FALSE(tree.OwnsPopup(combo));
	tree.Tick();
	REQUIRE_FALSE(tree.m_Animating);
}

TEST_CASE("Tab and slider logical changes do not wait for their visual transitions", "[controls][motion]")
{
	Yukiri::Detail::WidgetTree tree;
	auto tabs = std::make_unique<Yukiri::TabView>();
	auto* control = tabs.get();
	auto& first = tabs->AddTab("First", std::make_unique<Yukiri::Widget>());
	auto& second = tabs->AddTab("Second", std::make_unique<Yukiri::Widget>());
	tree.SetRoot(std::move(tabs));
	tree.Tick();
	tree.Layout({300, 200});
	int selected = 0;
	auto tabConnection = control->m_SelectionChanged.Connect([&](std::size_t) { ++selected; });
	REQUIRE(control->SetSelectedIndex(1));
	REQUIRE_FALSE(first.IsVisible());
	REQUIRE(second.IsVisible());
	REQUIRE(selected == 1);
	const auto now = std::chrono::steady_clock::now();
	tree.Tick(now + 40ms);
	REQUIRE(tree.m_Animating);
	tree.Tick(now + 1s);
	REQUIRE_FALSE(tree.m_Animating);
	auto slider = std::make_unique<Yukiri::Slider>();
	auto* value = slider.get();
	tree.SetRoot(std::move(slider));
	tree.Layout({200, 40});
	int changes = 0;
	auto sliderConnection = value->m_ValueChanged.Connect([&](float) { ++changes; });
	value->SetValue(1);
	REQUIRE(value->GetValue() == 1);
	tree.Tick(std::chrono::steady_clock::now() + 40ms);
	REQUIRE(tree.m_Animating);
	Yukiri::PointerEvent down{{192, 20}, Yukiri::PointerButton::Left};
	value->OnPointerDown(down);
	tree.Tick(std::chrono::steady_clock::now() + 1s);
	REQUIRE_FALSE(tree.m_Animating);
	REQUIRE(changes == 1);
	value->OnPointerUp(down);
}

TEST_CASE("Pointer focus does not reposition a tall clicked tab control", "[focus][scroll]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::ScrollView>();
	auto* view = root.get();
	auto content = std::make_unique<Yukiri::StackPanel>(Yukiri::Orientation::Vertical, 0.0f);
	content->AddChild<Yukiri::Widget>().SetHeight(120);
	auto& tabs = content->AddChild<Yukiri::TabView>();
	auto first = std::make_unique<Yukiri::Widget>();
	first->SetHeight(500);
	auto second = std::make_unique<Yukiri::Widget>();
	second->SetHeight(500);
	tabs.AddTab("One", std::move(first));
	tabs.AddTab("Two", std::move(second));
	view->SetContent(std::move(content));
	tree.SetRoot(std::move(root));
	tree.Layout({200, 300});
	tree.PointerDown({{100, 130}, Yukiri::PointerButton::Left});
	REQUIRE(tabs.GetSelectedIndex() == 1);
	REQUIRE(view->GetOffset().m_Y == 0);
}
