#include "AccessibilityTree.hpp"
#include "WidgetTree.hpp"
#include "Controls/ItemViewState.hpp"
#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Controls/ContextMenu.hpp>
#include <Yukiri/Controls/MenuBar.hpp>
#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Controls/TabView.hpp>
#include <Yukiri/Layout/SplitPanel.hpp>
#include <Yukiri/Platform/Window.hpp>

namespace Yukiri::Detail
{
AccessibilityTree::AccessibilityTree(WidgetTree& tree) : m_Tree(&tree), m_Lifetime(tree.m_Lifetime) {}
AccessibleNode AccessibilityTree::Root() const { return {AccessibleKind::Window, nullptr, m_Lifetime}; }
AccessibleNode AccessibilityTree::ForWidget(Widget& widget) const { return {AccessibleKind::Widget, &widget, widget.ObserveLifetime(), widget.m_AccessibilityId}; }
AccessibleNode AccessibilityTree::Part(AccessibleKind kind, Widget& owner, std::size_t index) const
{
	auto node = ForWidget(owner); node.m_Kind = kind; node.m_Index = index;
	if (auto* combo = dynamic_cast<ComboBox*>(&owner)) node.m_Generation = combo->m_ItemsGeneration;
	if (auto* menu = dynamic_cast<MenuBar*>(&owner)) node.m_Generation = menu->m_ItemsGeneration;
	return node;
}
AccessibleNode AccessibilityTree::ForItem(ListView& view, ItemId item, std::optional<std::size_t> column) const
{
	auto node = Part(column ? AccessibleKind::DataCell : AccessibleKind::DataRow, view, column.value_or(0));
	node.m_Item = item; node.m_Generation = view.m_Impl->m_ModelGeneration;
	return node;
}
bool AccessibilityTree::Active(Widget& widget) const
{
	if (widget.m_Tree != m_Tree) return false;
	const Widget* root = &widget;
	for (auto* current = &widget; current; current = current->GetParent())
	{
		if (current->GetAccessibilityInfo().m_Hidden) return false;
		root = current;
	}
	if (root == m_Tree->m_Root.get()) return true;
	for (const auto& popup : m_Tree->m_Popups) if (!popup->m_Closing && popup->m_Content.get() == root) return true;
	return false;
}
bool AccessibilityTree::IsAvailable(const AccessibleNode& node) const
{
	if (m_Lifetime.expired() || node.m_Lifetime.expired() || (m_Tree->m_Window && !m_Tree->m_Window->IsOpen())) return false;
	if (!node.m_Owner) return true;
	if (node.m_Owner->m_AccessibilityId != node.m_WidgetId || !Active(*node.m_Owner)) return false;
	switch (node.m_Kind)
	{
	case AccessibleKind::DataRow: case AccessibleKind::DataCell:
	{
		auto& view = static_cast<ListView&>(*node.m_Owner);
		if (view.m_Impl->m_ModelGeneration != node.m_Generation || !view.GetModel() || !view.GetModel()->Contains(node.m_Item) ||
			(node.m_Kind == AccessibleKind::DataCell && node.m_Index >= view.GetColumnCount())) return false;
		for (auto parent = view.GetModel()->GetParent(node.m_Item);; parent = view.GetModel()->GetParent(parent))
		{
			if (parent == view.GetRootItem()) return true;
			if (!parent) return false;
		}
	}
	case AccessibleKind::ComboItem:
	{
		auto& combo = static_cast<ComboBox&>(*node.m_Owner);
		return combo.m_ItemsGeneration == node.m_Generation && node.m_Index < combo.GetItems().size();
	}
	case AccessibleKind::MenuHeader:
	{
		auto& menu = static_cast<MenuBar&>(*node.m_Owner);
		return menu.m_ItemsGeneration == node.m_Generation && node.m_Index < menu.GetItems().size();
	}
	case AccessibleKind::MenuItem: return node.m_Index < static_cast<ContextMenu&>(*node.m_Owner).GetItems().size();
	case AccessibleKind::TabItem: return node.m_Index < static_cast<TabView&>(*node.m_Owner).m_Titles.size();
	case AccessibleKind::HeaderCell: return node.m_Index < static_cast<ListView&>(*node.m_Owner).GetColumnCount();
	case AccessibleKind::RadioGroup:
	{
		auto& radio = static_cast<RadioButton&>(*node.m_Owner);
		if (!radio.GetParent()) return false;
		for (const auto& child : radio.GetParent()->GetChildren())
			if (auto* sibling = dynamic_cast<RadioButton*>(child.get()); sibling && sibling->GetGroup() == radio.GetGroup() && sibling->IsVisible() && !sibling->GetAccessibilityInfo().m_Hidden) return sibling == &radio;
		return false;
	}
	default: return true;
	}
}
Widget* AccessibilityTree::PopupOwner(const Widget& widget) const
{
	for (const auto& popup : m_Tree->m_Popups) if (!popup->m_Closing && popup->m_Content.get() == &widget)
	{
		if (popup->m_Owner) return popup->m_Owner;
		if (popup->m_ParentId) for (const auto& parent : m_Tree->m_Popups) if (!parent->m_Closing && parent->m_Id == popup->m_ParentId) return parent->m_Content.get();
	}
	return nullptr;
}
std::vector<AccessibleNode> AccessibilityTree::WidgetChildren(Widget& widget) const
{
	std::vector<AccessibleNode> result;
	std::vector<std::string> groups;
	for (const auto& child : widget.GetChildren())
	{
		if (!child->IsVisible() || child->GetAccessibilityInfo().m_Hidden) continue;
		if (auto* radio = dynamic_cast<RadioButton*>(child.get()))
		{
			const std::string group(radio->GetGroup());
			if (std::ranges::find(groups, group) == groups.end()) { groups.push_back(group); result.push_back(Part(AccessibleKind::RadioGroup, *radio)); }
		}
		else result.push_back(ForWidget(*child));
	}
	if (dynamic_cast<SplitPanel*>(&widget)) result.push_back(Part(AccessibleKind::SplitGrip, widget));
	for (const auto& popup : m_Tree->m_Popups) if (!popup->m_Closing && popup->m_Content)
	{
		const auto node = ForWidget(*popup->m_Content);
		const auto parent = Parent(node);
		if (parent && *parent == ForWidget(widget)) result.push_back(node);
	}
	return result;
}
std::size_t AccessibilityTree::ChildCount(const AccessibleNode& node) const
{
	if (!IsAvailable(node)) return 0;
	if (node.m_Kind == AccessibleKind::Window)
	{
		std::size_t count = m_Tree->m_Window && m_Tree->m_Window->GetConfig().m_Decoration == WindowDecoration::Custom ? 1 : 0;
		if (m_Tree->m_Root && m_Tree->m_Root->IsVisible() && !m_Tree->m_Root->GetAccessibilityInfo().m_Hidden) ++count;
		for (const auto& popup : m_Tree->m_Popups) if (!popup->m_Closing && popup->m_Content)
			if (const auto parent = Parent(ForWidget(*popup->m_Content)); parent && parent->m_Kind == AccessibleKind::Window) ++count;
		return count;
	}
	if (node.m_Kind == AccessibleKind::TitleBar) return 3;
	if (node.m_Kind == AccessibleKind::HeaderRow || node.m_Kind == AccessibleKind::DataRow || node.m_Kind == AccessibleKind::DataCell)
	{
		auto& view = static_cast<ListView&>(*node.m_Owner);
		if (node.m_Kind == AccessibleKind::HeaderRow) return view.GetColumnCount();
		if (node.m_Kind == AccessibleKind::DataCell) return view.GetEditingItem() == node.m_Item && view.GetEditingColumn() == node.m_Index && view.GetEditor() ? 1 : 0;
		if (view.m_Impl->m_Kind == ListView::ViewKind::Table) return view.GetColumnCount();
		const auto children = view.m_Impl->m_Kind == ListView::ViewKind::Tree && view.IsExpanded(node.m_Item) ? view.GetModel()->GetRowCount(node.m_Item) : 0;
		return children + (view.GetEditingItem() == node.m_Item && view.GetEditor() ? 1 : 0);
	}
	if (node.m_Kind == AccessibleKind::RadioGroup)
	{
		auto& radio = static_cast<RadioButton&>(*node.m_Owner);
		std::size_t count = 0;
		for (const auto& child : radio.GetParent()->GetChildren()) if (auto* peer = dynamic_cast<RadioButton*>(child.get()); peer && peer->GetGroup() == radio.GetGroup() && peer->IsVisible() && !peer->GetAccessibilityInfo().m_Hidden) ++count;
		return count;
	}
	if (node.m_Kind == AccessibleKind::TabItem)
	{
		auto& tabs = static_cast<TabView&>(*node.m_Owner);
		return node.m_Index == tabs.GetSelectedIndex() && node.m_Index < tabs.GetChildren().size() && tabs.GetChildren()[node.m_Index]->IsVisible() ? 1 : 0;
	}
	if (node.m_Kind == AccessibleKind::MenuHeader)
	{
		auto& bar = static_cast<MenuBar&>(*node.m_Owner);
		return bar.m_Current == node.m_Index && !bar.m_MenuLifetime.expired() && bar.IsPopupOpen() ? 1 : 0;
	}
	if (node.m_Kind == AccessibleKind::MenuItem)
	{
		auto& menu = static_cast<ContextMenu&>(*node.m_Owner);
		return menu.m_Current == node.m_Index && !menu.m_ChildLifetime.expired() && menu.m_Child->IsPopupOpen() ? 1 : 0;
	}
	if (node.m_Kind != AccessibleKind::Widget) return 0;
	if (auto* view = dynamic_cast<ListView*>(node.m_Owner))
		return !view->GetModel() ? 0 : view->GetModel()->GetRowCount(view->GetRootItem()) + (view->m_Impl->m_Kind == ListView::ViewKind::Table ? 1 : 0);
	if (auto* tabs = dynamic_cast<TabView*>(node.m_Owner)) return tabs->m_Titles.size();
	if (auto* combo = dynamic_cast<ComboBox*>(node.m_Owner)) return combo->IsExpanded() ? combo->GetItems().size() : 0;
	if (auto* menu = dynamic_cast<ContextMenu*>(node.m_Owner)) return menu->GetItems().size();
	if (auto* bar = dynamic_cast<MenuBar*>(node.m_Owner)) return bar->GetItems().size();
	return WidgetChildren(*node.m_Owner).size();
}
std::optional<AccessibleNode> AccessibilityTree::Child(const AccessibleNode& node, std::size_t index) const
{
	if (index >= ChildCount(node)) return {};
	if (node.m_Kind == AccessibleKind::Window)
	{
		if (m_Tree->m_Window && m_Tree->m_Window->GetConfig().m_Decoration == WindowDecoration::Custom)
		{
			if (index == 0) { auto title = Root(); title.m_Kind = AccessibleKind::TitleBar; return title; }
			--index;
		}
		if (m_Tree->m_Root && m_Tree->m_Root->IsVisible() && !m_Tree->m_Root->GetAccessibilityInfo().m_Hidden)
		{ if (index == 0) return ForWidget(*m_Tree->m_Root); --index; }
		for (const auto& popup : m_Tree->m_Popups) if (!popup->m_Closing && popup->m_Content)
		{
			const auto child = ForWidget(*popup->m_Content);
			const auto parent = Parent(child);
			if (parent && parent->m_Kind == AccessibleKind::Window) { if (index == 0) return child; --index; }
		}
		return {};
	}
	if (node.m_Kind == AccessibleKind::TitleBar) { auto button = Root(); button.m_Kind = AccessibleKind::CaptionButton; button.m_Index = index; return button; }
	if (node.m_Kind == AccessibleKind::HeaderRow) return Part(AccessibleKind::HeaderCell, *node.m_Owner, index);
	if (node.m_Kind == AccessibleKind::DataCell) return ForWidget(*static_cast<ListView&>(*node.m_Owner).GetEditor());
	if (node.m_Kind == AccessibleKind::DataRow)
	{
		auto& view = static_cast<ListView&>(*node.m_Owner);
		if (view.m_Impl->m_Kind == ListView::ViewKind::Table) return ForItem(view, node.m_Item, index);
		const auto children = view.IsExpanded(node.m_Item) ? view.GetModel()->GetRowCount(node.m_Item) : 0;
		return index < children ? ForItem(view, view.GetModel()->GetChild(node.m_Item, index)) : ForWidget(*view.GetEditor());
	}
	if (node.m_Kind == AccessibleKind::RadioGroup)
	{
		auto& radio = static_cast<RadioButton&>(*node.m_Owner);
		for (const auto& child : radio.GetParent()->GetChildren()) if (auto* peer = dynamic_cast<RadioButton*>(child.get()); peer && peer->GetGroup() == radio.GetGroup() && peer->IsVisible() && !peer->GetAccessibilityInfo().m_Hidden)
		{ if (index == 0) return ForWidget(*peer); --index; }
		return {};
	}
	if (node.m_Kind == AccessibleKind::TabItem) return ForWidget(*node.m_Owner->GetChildren()[node.m_Index]);
	if (node.m_Kind == AccessibleKind::MenuItem) return ForWidget(*static_cast<ContextMenu&>(*node.m_Owner).m_Child);
	if (node.m_Kind == AccessibleKind::MenuHeader) return ForWidget(*static_cast<MenuBar&>(*node.m_Owner).m_Menu);
	if (auto* view = dynamic_cast<ListView*>(node.m_Owner))
	{
		if (view->m_Impl->m_Kind == ListView::ViewKind::Table) { if (index == 0) return Part(AccessibleKind::HeaderRow, *view); --index; }
		return ForItem(*view, view->GetModel()->GetChild(view->GetRootItem(), index));
	}
	if (dynamic_cast<TabView*>(node.m_Owner)) return Part(AccessibleKind::TabItem, *node.m_Owner, index);
	if (dynamic_cast<ComboBox*>(node.m_Owner)) return Part(AccessibleKind::ComboItem, *node.m_Owner, index);
	if (dynamic_cast<ContextMenu*>(node.m_Owner)) return Part(AccessibleKind::MenuItem, *node.m_Owner, index);
	if (dynamic_cast<MenuBar*>(node.m_Owner)) return Part(AccessibleKind::MenuHeader, *node.m_Owner, index);
	return WidgetChildren(*node.m_Owner)[index];
}
std::optional<AccessibleNode> AccessibilityTree::Parent(const AccessibleNode& node) const
{
	if (!IsAvailable(node) || node.m_Kind == AccessibleKind::Window) return {};
	if (node.m_Kind == AccessibleKind::TitleBar) return Root();
	if (node.m_Kind == AccessibleKind::CaptionButton) { auto title = Root(); title.m_Kind = AccessibleKind::TitleBar; return title; }
	if (node.m_Kind == AccessibleKind::DataCell) return ForItem(static_cast<ListView&>(*node.m_Owner), node.m_Item);
	if (node.m_Kind == AccessibleKind::HeaderCell) return Part(AccessibleKind::HeaderRow, *node.m_Owner);
	if (node.m_Kind == AccessibleKind::DataRow)
	{
		auto& view = static_cast<ListView&>(*node.m_Owner);
		const auto parent = view.GetModel()->GetParent(node.m_Item);
		return parent == view.GetRootItem() || view.m_Impl->m_Kind != ListView::ViewKind::Tree ? ForWidget(view) : ForItem(view, parent);
	}
	if (node.m_Kind == AccessibleKind::RadioGroup) return ForWidget(*node.m_Owner->GetParent());
	if (node.m_Kind != AccessibleKind::Widget) return ForWidget(*node.m_Owner);
	if (auto* parent = node.m_Owner->GetParent())
	{
		if (auto* view = dynamic_cast<ListView*>(parent); view && view->GetEditor() == node.m_Owner)
			return ForItem(*view, view->GetEditingItem(), view->m_Impl->m_Kind == ListView::ViewKind::Table ? std::optional(view->GetEditingColumn()) : std::nullopt);
		if (auto* tabs = dynamic_cast<TabView*>(parent))
			for (std::size_t index = 0; index < tabs->GetChildren().size(); ++index) if (tabs->GetChildren()[index].get() == node.m_Owner) return Part(AccessibleKind::TabItem, *tabs, index);
		if (auto* radio = dynamic_cast<RadioButton*>(node.m_Owner))
			for (const auto& child : parent->GetChildren()) if (auto* peer = dynamic_cast<RadioButton*>(child.get()); peer && peer->GetGroup() == radio->GetGroup() && peer->IsVisible() && !peer->GetAccessibilityInfo().m_Hidden) return Part(AccessibleKind::RadioGroup, *peer);
		return ForWidget(*parent);
	}
	if (auto* owner = PopupOwner(*node.m_Owner))
	{
		if (dynamic_cast<ContextMenu*>(node.m_Owner))
		{
			if (auto* menu = dynamic_cast<ContextMenu*>(owner); menu && menu->m_Current) return Part(AccessibleKind::MenuItem, *menu, *menu->m_Current);
			if (auto* bar = dynamic_cast<MenuBar*>(owner)) return Part(AccessibleKind::MenuHeader, *bar, bar->m_Current);
		}
		for (const auto& popup : m_Tree->m_Popups) if (popup->m_Content.get() == node.m_Owner && popup->m_Modal && popup->m_ParentId) return ForWidget(*owner);
	}
	return Root();
}
std::size_t AccessibilityTree::Ordinal(const AccessibleNode& node) const
{
	if (node.m_Kind == AccessibleKind::DataRow)
	{
		auto& view = static_cast<ListView&>(*node.m_Owner);
		std::size_t row = 0;
		if (const auto index = view.GetRowIndex(node.m_Item)) row = view.m_Impl->m_Rows[*index].m_Ordinal;
		else for (const auto count = view.GetModel()->GetRowCount(view.GetModel()->GetParent(node.m_Item)); row < count; ++row)
			if (view.GetModel()->GetChild(view.GetModel()->GetParent(node.m_Item), row) == node.m_Item) break;
		return row + (view.m_Impl->m_Kind == ListView::ViewKind::Table ? 1 : 0);
	}
	if (node.m_Kind == AccessibleKind::DataCell || node.m_Kind == AccessibleKind::HeaderCell || node.m_Kind == AccessibleKind::CaptionButton ||
		node.m_Kind == AccessibleKind::TabItem || node.m_Kind == AccessibleKind::ComboItem || node.m_Kind == AccessibleKind::MenuItem || node.m_Kind == AccessibleKind::MenuHeader) return node.m_Index;
	if (node.m_Kind == AccessibleKind::HeaderRow) return 0;
	const auto parent = Parent(node);
	if (!parent) return 0;
	for (std::size_t index = 0, count = ChildCount(*parent); index < count; ++index) if (const auto child = Child(*parent, index); child && *child == node) return index;
	return ChildCount(*parent);
}
std::optional<AccessibleNode> AccessibilityTree::Sibling(const AccessibleNode& node, bool next) const
{
	const auto parent = Parent(node);
	if (!parent) return {};
	const auto ordinal = Ordinal(node);
	if (!next && ordinal == 0) return {};
	return Child(*parent, next ? ordinal + 1 : ordinal - 1);
}
void AccessibilityTree::Dispatch(std::function<void()> action) const { if (!m_Lifetime.expired()) m_Tree->Invoke(std::move(action)); }
void AccessibilityTree::EnsureLayout() const
{
	if (m_Lifetime.expired()) return;
	if (m_Tree->m_Window)
	{
		const auto content = m_Tree->m_Window->GetContentBounds();
		m_Tree->Layout({content.m_Width, content.m_Height}, {content.m_X, content.m_Y});
	}
	else if (m_Tree->m_LastSize.m_Width >= 0) m_Tree->Layout(m_Tree->m_LastSize, m_Tree->m_LastOrigin);
}
void AccessibilityTree::StructureChanged(Widget& widget)
{
	if (widget.m_Tree) { ++widget.m_Tree->m_AccessibilityStructureVersion; widget.InvalidatePaint(); }
}
std::uint64_t AccessibilityTree::StructureVersion() const { return m_Lifetime.expired() ? 0 : m_Tree->m_AccessibilityStructureVersion; }
bool AccessibilityTree::HasCustomTitleBar() const { return !m_Lifetime.expired() && m_Tree->m_Window && m_Tree->m_Window->GetConfig().m_Decoration == WindowDecoration::Custom; }
}
