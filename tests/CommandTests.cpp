#include "UI/WidgetTree.hpp"
#include "Platform/CloseState.hpp"
#include <Yukiri/Controls/Button.hpp>
#include <Yukiri/Controls/TextBox.hpp>
#include <Yukiri/Controls/CheckBox.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Command scopes respect focused editing, disabled commands, repeats, and AltGr", "[commands]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	auto& editor = root->AddChild<Yukiri::TextBox>("text");
	auto local = std::make_shared<Yukiri::Command>("Save locally");
	local->SetShortcut(Yukiri::KeyGesture{Yukiri::Key::S, {false, true, false}});
	auto global = std::make_shared<Yukiri::Command>("Save globally");
	global->SetShortcut(local->GetShortcut());
	int localCalls = 0;
	int globalCalls = 0;
	auto lc = local->m_Executed.Connect([&] { ++localCalls; });
	auto gc = global->m_Executed.Connect([&] { ++globalCalls; });
	editor.AddCommand(local);
	tree.m_Commands.push_back(global);
	tree.SetRoot(std::move(root));
	tree.SetFocus(&editor);
	tree.KeyDown({Yukiri::Key::S, {false, true, false}});
	REQUIRE(localCalls == 1);
	REQUIRE(globalCalls == 0);
	tree.KeyDown({Yukiri::Key::S, {false, true, false}, true});
	REQUIRE(localCalls == 1);
	local->SetEnabled(false);
	tree.KeyDown({Yukiri::Key::S, {false, true, false}});
	REQUIRE(globalCalls == 1);
	tree.KeyDown({Yukiri::Key::S, {false, true, true, false, true}});
	REQUIRE(globalCalls == 1);
	global->SetShortcut(Yukiri::KeyGesture{Yukiri::Key::A, {false, true, false}});
	tree.KeyDown({Yukiri::Key::A, {false, true, false}});
	REQUIRE(globalCalls == 1);
}

TEST_CASE("Space activates on release and capture cancellation disarms buttons", "[input]")
{
	Yukiri::Detail::WidgetTree tree;
	auto button = std::make_unique<Yukiri::Button>();
	auto command = std::make_shared<Yukiri::Command>("Bound action");
	int calls = 0;
	auto connection = command->m_Executed.Connect([&] { ++calls; });
	button->SetCommand(command);
	auto* control = button.get();
	tree.SetRoot(std::move(button));
	tree.Layout({100, 40});
	tree.SetFocus(control);
	tree.KeyDown({Yukiri::Key::Space});
	REQUIRE(calls == 0);
	tree.KeyUp({Yukiri::Key::Space});
	REQUIRE(calls == 1);
	tree.PointerDown({{10, 10}, Yukiri::PointerButton::Left});
	tree.CancelPointer();
	tree.PointerUp({{10, 10}, Yukiri::PointerButton::Left});
	REQUIRE(calls == 1);
	command->SetEnabled(false);
	REQUIRE_FALSE(control->IsEnabled());
	REQUIRE_FALSE(tree.m_Focus);
}

TEST_CASE("Close requests coalesce, can be vetoed, and accept a later retry", "[window]")
{
	Yukiri::Detail::CloseState state;
	Yukiri::Signal<Yukiri::CloseRequestEvent&> requested;
	int calls = 0;
	auto connection = requested.Connect([&](auto& event) { event.m_Cancel = ++calls == 1; });
	state.Request();
	state.Request();
	state.Process(requested);
	REQUIRE(state.m_Open);
	REQUIRE_FALSE(state.m_Pending);
	REQUIRE(calls == 1);
	state.Request();
	state.Process(requested);
	REQUIRE_FALSE(state.m_Open);
	state.Request();
	state.Process(requested);
	REQUIRE(calls == 2);
}

TEST_CASE("Window activation restores live focus and forgets destroyed descendants", "[focus]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	auto& button = root->AddChild<Yukiri::Button>("Focus");
	auto* container = root.get();
	tree.SetRoot(std::move(root));
	tree.SetFocus(&button);
	tree.LostFocus();
	REQUIRE_FALSE(tree.m_Focus);
	tree.GainedFocus();
	REQUIRE(tree.m_Focus == &button);
	REQUIRE_FALSE(button.IsFocusVisible());
	tree.LostFocus();
	container->ClearChildren();
	tree.GainedFocus();
	REQUIRE_FALSE(tree.m_Focus);
}

TEST_CASE("Checkable command state stays synchronized with bound controls", "[commands]")
{
	auto command = std::make_shared<Yukiri::Command>("Option");
	command->SetCheckable(true);
	command->SetChecked(true);
	Yukiri::Detail::WidgetTree tree;
	auto owned = std::make_unique<Yukiri::CheckBox>();
	auto& box = *owned;
	box.SetCommand(command);
	tree.SetRoot(std::move(owned));
	tree.Layout({100, 40});
	REQUIRE(box.IsChecked());
	Yukiri::KeyEvent pressed{Yukiri::Key::Space};
	Yukiri::KeyEvent released{Yukiri::Key::Space};
	box.OnKeyDown(pressed);
	box.OnKeyUp(released);
	REQUIRE_FALSE(command->IsChecked());
	REQUIRE_FALSE(box.IsChecked());
	command->SetChecked(true);
	REQUIRE(box.IsChecked());
	REQUIRE_FALSE(tree.m_LayoutDirty);
}
