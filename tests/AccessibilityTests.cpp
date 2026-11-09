#include "UI/AccessibilityTree.hpp"
#include "UI/WidgetTree.hpp"
#include <Yukiri/Controls/Button.hpp>
#include <Yukiri/Controls/Label.hpp>
#include <Yukiri/Controls/TextArea.hpp>
#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Controls/TabView.hpp>
#include <Yukiri/Controls/TreeView.hpp>
#include <Yukiri/Data/StandardItemModel.hpp>
#include <Yukiri/Layout/StackPanel.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Accessibility names, label associations, grouping, and destroyed nodes follow ownership", "[accessibility]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::StackPanel>(); auto* panel = root.get();
	auto& label = root->AddChild<Yukiri::Label>("Account name");
	auto& text = root->AddChild<Yukiri::TextBox>();
	Yukiri::AccessibilityInfo info; info.m_Label = &label; info.m_AutomationId = "Account"; text.SetAccessibilityInfo(info);
	auto& first = root->AddChild<Yukiri::RadioButton>("First");
	auto& second = root->AddChild<Yukiri::RadioButton>("Second");
	tree.SetRoot(std::move(root)); tree.Layout({400, 300});
	Yukiri::Detail::AccessibilityTree accessibility(tree);
	const auto node = accessibility.ForWidget(text);
	REQUIRE(accessibility.Describe(node).m_Info.m_Name == "Account name");
	const auto group = accessibility.Parent(accessibility.ForWidget(first)); REQUIRE(group);
	REQUIRE(group->m_Kind == Yukiri::Detail::AccessibleKind::RadioGroup);
	REQUIRE(accessibility.ChildCount(*group) == 2);
	REQUIRE(accessibility.Select(accessibility.ForWidget(second)));
	REQUIRE(second.IsChecked()); REQUIRE(accessibility.Selection(*group).size() == 1);
	panel->RemoveChild(label);
	REQUIRE(text.GetAccessibilityInfo().m_Label == nullptr);
	panel->RemoveChild(text);
	REQUIRE_FALSE(accessibility.IsAvailable(node));
}

TEST_CASE("Accessibility tab semantics include headers and the active page", "[accessibility][tabs]")
{
	Yukiri::Detail::WidgetTree tree;
	auto tabs = std::make_unique<Yukiri::TabView>(); auto* control = tabs.get();
	tabs->AddTab("One", std::make_unique<Yukiri::Button>("First page"));
	tabs->AddTab("Two", std::make_unique<Yukiri::Button>("Second page"));
	tree.SetRoot(std::move(tabs)); tree.Layout({400, 200});
	Yukiri::Detail::AccessibilityTree accessibility(tree);
	const auto owner = accessibility.ForWidget(*control);
	REQUIRE(accessibility.ChildCount(owner) == 2);
	const auto first = accessibility.Child(owner, 0), second = accessibility.Child(owner, 1); REQUIRE(first); REQUIRE(second);
	REQUIRE(accessibility.ChildCount(*first) == 1); REQUIRE(accessibility.ChildCount(*second) == 0);
	REQUIRE(accessibility.Select(*second));
	REQUIRE(accessibility.ChildCount(*first) == 0); REQUIRE(accessibility.ChildCount(*second) == 1);
	REQUIRE(accessibility.Parent(*accessibility.Child(*second, 0)) == second);
}

TEST_CASE("Accessibility virtual rows remain addressable when collapsed and invalidate on model replacement", "[accessibility][data]")
{
	auto model = std::make_shared<Yukiri::StandardItemModel>();
	Yukiri::ItemModel::Cell cell; cell.m_Text = "Parent";
	auto parent = model->AppendRow({cell}); REQUIRE(parent);
	cell.m_Text = "Child"; auto child = model->AppendRow({cell}, *parent); REQUIRE(child);
	auto view = std::make_unique<Yukiri::TreeView>(); auto* control = view.get(); view->SetModel(model);
	Yukiri::Detail::WidgetTree tree; tree.SetRoot(std::move(view)); tree.Layout({300, 180});
	Yukiri::Detail::AccessibilityTree accessibility(tree);
	const auto node = accessibility.ForItem(*control, *child);
	REQUIRE(accessibility.IsAvailable(node)); REQUIRE_FALSE(accessibility.Describe(node).m_Visible);
	REQUIRE(accessibility.Realize(node)); REQUIRE(control->IsExpanded(*parent));
	REQUIRE(accessibility.Describe(node).m_Visible);
	control->SetModel(std::make_shared<Yukiri::StandardItemModel>());
	REQUIRE_FALSE(accessibility.IsAvailable(node));
}
