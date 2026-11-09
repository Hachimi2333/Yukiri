#include <Yukiri/Controls/ContextMenu.hpp>
#include <Yukiri/Controls/MenuBar.hpp>
#include <Yukiri/Controls/Button.hpp>
#include "UI/WidgetTree.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Menu navigation skips separators and disabled actions and restores parent focus", "[menu]")
{
	using Menu = Yukiri::ContextMenu;
	auto disabled = std::make_shared<Yukiri::Command>("Disabled");
	disabled->SetEnabled(false);
	auto action = std::make_shared<Yukiri::Command>("Action");
	int invoked = 0;
	auto connection = action->m_Executed.Connect([&] { ++invoked; });
	auto menu = std::make_unique<Menu>(std::vector<Menu::Item>{Menu::Item::Action(disabled), Menu::Item::Divider(),
		Menu::Item::Submenu("Nested", {Menu::Item::Action(action)}), Menu::Item::Action(action)});
	auto* root = menu.get();
	Yukiri::Detail::WidgetTree tree;
	tree.m_Theme.m_Motion.m_Enabled = false;
	tree.SetRoot(std::make_unique<Yukiri::Button>("Origin"));
	tree.Layout({600, 400});
	tree.SetFocus(tree.m_Root.get());
	REQUIRE(tree.OpenPopup(tree.m_Root.get(), std::move(menu), {}));
	tree.Layout({600, 400});
	REQUIRE(root->GetCurrentItem() == 2);
	tree.KeyDown({Yukiri::Key::Right});
	tree.Layout({600, 400});
	REQUIRE(tree.GetPopupRoots().size() == 2);
	REQUIRE(tree.m_Focus != root);
	tree.KeyDown({Yukiri::Key::Left});
	REQUIRE(root->HasFocus());
	REQUIRE(tree.GetPopupRoots().size() == 1);
	tree.KeyDown({Yukiri::Key::Down});
	REQUIRE(root->GetCurrentItem() == 3);
	tree.KeyDown({Yukiri::Key::Enter});
	REQUIRE(invoked == 1);
	REQUIRE(tree.GetPopupRoots().empty());
	REQUIRE(tree.m_Root->HasFocus());
}

TEST_CASE("Context menu requests route from pointer release and keyboard without changing clipboard", "[menu][input]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	auto& child = root->AddChild<Yukiri::Button>("Target");
	int requests = 0;
	auto connection = root->m_ContextMenuRequested.Connect([&](auto) { ++requests; });
	tree.SetRoot(std::move(root));
	tree.Layout({300, 100});
	tree.PointerDown({{20, 20}, Yukiri::PointerButton::Right});
	tree.PointerUp({{20, 20}, Yukiri::PointerButton::Right});
	REQUIRE(child.HasFocus());
	REQUIRE(requests == 1);
	tree.KeyDown({Yukiri::Key::Menu});
	tree.KeyDown({Yukiri::Key::F10, {true}});
	REQUIRE(requests == 3);
}

TEST_CASE("Modal scopes reject foreign popups and suppress window shortcuts", "[popup][command]")
{
	Yukiri::Detail::WidgetTree tree;
	tree.m_Theme.m_Motion.m_Enabled = false;
	tree.SetRoot(std::make_unique<Yukiri::Button>("Outside"));
	auto command = std::make_shared<Yukiri::Command>();
	command->SetShortcut(Yukiri::KeyGesture{Yukiri::Key::F1});
	int invoked = 0;
	auto connection = command->m_Executed.Connect([&] { ++invoked; });
	tree.m_Commands.push_back(command);
	REQUIRE(tree.OpenPopup(nullptr, std::make_unique<Yukiri::Button>("Modal"), {}, true));
	REQUIRE_FALSE(tree.OpenPopup(tree.m_Root.get(), std::make_unique<Yukiri::Button>("Foreign"), {}));
	tree.KeyDown({Yukiri::Key::F1});
	REQUIRE(invoked == 0);
	tree.CloseAllPopups();
	tree.KeyDown({Yukiri::Key::F1});
	REQUIRE(invoked == 1);
}
