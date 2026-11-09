#include "AccessibilityTree.hpp"
#include "WidgetTree.hpp"
#include "Controls/ItemViewState.hpp"
#include "Controls/TextBoxState.hpp"
#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Controls/ContextMenu.hpp>
#include <Yukiri/Controls/Dialog.hpp>
#include <Yukiri/Controls/ImageView.hpp>
#include <Yukiri/Controls/Label.hpp>
#include <Yukiri/Controls/MenuBar.hpp>
#include <Yukiri/Controls/NumberBox.hpp>
#include <Yukiri/Controls/ProgressBar.hpp>
#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Controls/Separator.hpp>
#include <Yukiri/Controls/Slider.hpp>
#include <Yukiri/Controls/StatusBar.hpp>
#include <Yukiri/Controls/TabView.hpp>
#include <Yukiri/Controls/ToggleSwitch.hpp>
#include <Yukiri/Controls/ToolBar.hpp>
#include <Yukiri/Controls/ToolTip.hpp>
#include <Yukiri/Controls/ScrollView.hpp>
#include <Yukiri/Layout/SplitPanel.hpp>
#include <Yukiri/Platform/Window.hpp>

namespace Yukiri::Detail
{
namespace
{
using Role = AccessibilityInfo::Role;
std::string Caption(std::string_view text)
{
	std::string result;
	for (std::size_t index = 0; index < text.size(); ++index) { if (text[index] == '&' && index + 1 < text.size()) { if (text[index + 1] == '&') ++index; else continue; } result += text[index]; }
	return result;
}
std::string Gesture(const std::optional<KeyGesture>& gesture)
{
	if (!gesture) return {};
	std::string text;
	if (gesture->m_Modifiers.m_Control) text += "Ctrl+";
	if (gesture->m_Modifiers.m_Alt) text += "Alt+";
	if (gesture->m_Modifiers.m_Shift) text += "Shift+";
	if (gesture->m_Key >= Key::A && gesture->m_Key <= Key::Z) text += static_cast<char>('A' + static_cast<int>(gesture->m_Key) - static_cast<int>(Key::A));
	else if (gesture->m_Key >= Key::F1 && gesture->m_Key <= Key::F24) text += "F" + std::to_string(static_cast<int>(gesture->m_Key) - static_cast<int>(Key::F1) + 1);
	else if (gesture->m_Key >= Key::D0 && gesture->m_Key <= Key::D9) text += static_cast<char>('0' + static_cast<int>(gesture->m_Key) - static_cast<int>(Key::D0));
	else text += "Key";
	return text;
}
std::string PlainName(Widget& widget)
{
	if (const auto info = widget.GetAccessibilityInfo(); !info.m_Name.empty()) return info.m_Name;
	if (const auto* label = dynamic_cast<Label*>(&widget)) return label->GetText();
	if (const auto* button = dynamic_cast<Button*>(&widget)) return button->GetText();
	return {};
}
}
bool AccessibilityTree::Enabled(Widget& widget) const
{
	for (auto* current = &widget; current; current = current->GetParent()) if (!current->IsEnabled()) return false;
	if (auto* modal = m_Tree->GetModalRoot(); modal && !WidgetTree::Within(&widget, modal)) return false;
	return true;
}
Rect AccessibilityTree::Clip(Widget& widget, Rect bounds) const
{
	if (m_Tree->m_Window && (!m_Tree->m_Window->IsVisible() || m_Tree->m_Window->IsMinimized())) return {};
	for (auto* current = &widget; current; current = current->GetParent())
	{
		if (!current->IsVisible() || current->GetAccessibilityInfo().m_Hidden) return {};
		if (current != &widget) bounds = bounds.Intersect(current->GetChildrenClip());
	}
	for (const auto& popup : m_Tree->m_Popups) if (!popup->m_Closing && popup->m_Content)
		if (WidgetTree::Within(&widget, popup->m_Content.get())) bounds = bounds.Intersect(m_Tree->VisibleBounds(*popup));
	return bounds.Intersect({m_Tree->m_LastOrigin.m_X, m_Tree->m_LastOrigin.m_Y, std::max(0.0f, m_Tree->m_LastSize.m_Width), std::max(0.0f, m_Tree->m_LastSize.m_Height)});
}
AccessibleState AccessibilityTree::Describe(const AccessibleNode& node) const
{
	AccessibleState state;
	if (!IsAvailable(node)) { state.m_Enabled = state.m_Visible = false; return state; }
	auto add = [&](AccessiblePattern pattern) { state.m_Patterns |= static_cast<std::uint64_t>(pattern); };
	if (!node.m_Owner)
	{
		const auto* window = m_Tree->m_Window;
		const auto size = window ? window->GetSize() : Size{m_Tree->m_LastSize.m_Width, m_Tree->m_LastSize.m_Height + m_Tree->m_LastOrigin.m_Y};
		state.m_Visible = !window || (window->IsVisible() && !window->IsMinimized());
		state.m_Info.m_Role = node.m_Kind == AccessibleKind::Window ? Role::Window : node.m_Kind == AccessibleKind::TitleBar ? Role::TitleBar : Role::Button;
		state.m_Info.m_Name = window ? window->GetConfig().m_Title : "Yukiri window";
		state.m_Bounds = {0, 0, size.m_Width, node.m_Kind == AccessibleKind::Window ? size.m_Height : 40};
		if (node.m_Kind == AccessibleKind::Window) { add(AccessiblePattern::Window); state.m_Focusable = true; state.m_Focused = false; if (m_Tree->GetModalRoot()) state.m_ItemStatus = "Blocked"; }
		if (node.m_Kind == AccessibleKind::CaptionButton)
		{
			state.m_Info.m_Name = node.m_Index == 0 ? "Minimize" : node.m_Index == 1 ? (window && window->IsMaximized() ? "Restore" : "Maximize") : "Close";
			state.m_Bounds = {std::max(0.0f, size.m_Width - 138 + static_cast<float>(node.m_Index) * 46), 0, std::min(46.0f, size.m_Width), 40};
			add(AccessiblePattern::Invoke);
		}
		if (!state.m_Visible) state.m_Bounds = {};
		return state;
	}
	auto& widget = *node.m_Owner;
	state.m_Info = widget.GetAccessibilityInfo();
	if (node.m_Kind != AccessibleKind::Widget)
	{
		state.m_Info.m_AutomationId = (state.m_Info.m_AutomationId.empty() ? "Yukiri." + std::to_string(node.m_WidgetId) : state.m_Info.m_AutomationId) + "." +
			std::to_string(static_cast<int>(node.m_Kind)) + "." + std::to_string(node.m_Item) + "." + std::to_string(node.m_Index);
		state.m_Info.m_Label = nullptr; state.m_Info.m_IsContentElement.reset();
	}
	if (state.m_Info.m_Name.empty() && state.m_Info.m_Label) state.m_Info.m_Name = PlainName(*state.m_Info.m_Label);
	if (state.m_Info.m_Description.empty()) state.m_Info.m_Description = widget.GetToolTip();
	state.m_Enabled = Enabled(widget);
	state.m_Bounds = Clip(widget, widget.GetBounds());
	state.m_Focusable = widget.IsFocusable();
	state.m_Focused = widget.HasFocus();
	state.m_Visible = !state.m_Bounds.Empty() && (!m_Tree->m_Window || (m_Tree->m_Window->IsVisible() && !m_Tree->m_Window->IsMinimized()));
	if (node.m_Kind == AccessibleKind::DataRow || node.m_Kind == AccessibleKind::DataCell)
	{
		auto& view = static_cast<ListView&>(widget);
		const auto column = node.m_Kind == AccessibleKind::DataCell ? node.m_Index : 0;
		const auto cell = view.GetModel()->GetCell(node.m_Item, column);
		state.m_Info.m_Role = view.m_Impl->m_Kind == ListView::ViewKind::Tree ? Role::TreeItem : view.m_Impl->m_Kind == ListView::ViewKind::Table ? Role::DataItem : Role::ListItem;
		state.m_Info.m_Name = cell.m_Text; state.m_Info.m_Label = nullptr;
		state.m_Value = cell.m_Text;
		state.m_Bounds = Clip(view, view.GetItemBounds(node.m_Item, column));
		state.m_Visible = !state.m_Bounds.Empty();
		state.m_Enabled = state.m_Enabled && cell.m_Enabled;
		state.m_Focusable = cell.m_Selectable;
		state.m_Selected = view.GetSelectionModel()->IsSelected(node.m_Item);
		state.m_Focused = view.HasFocus() && view.GetSelectionModel()->GetCurrent() == node.m_Item &&
			(view.m_Impl->m_Kind != ListView::ViewKind::Table || (node.m_Kind == AccessibleKind::DataCell && view.GetCurrentColumn() == node.m_Index));
		state.m_ReadOnly = !cell.m_Editable;
		state.m_Checked = cell.m_Checked.value_or(false);
		add(AccessiblePattern::SelectionItem); add(AccessiblePattern::ScrollItem); add(AccessiblePattern::VirtualizedItem);
		if (node.m_Kind == AccessibleKind::DataCell) { add(AccessiblePattern::GridItem); add(AccessiblePattern::TableItem); }
		if (view.m_Impl->m_Kind != ListView::ViewKind::Table || node.m_Kind == AccessibleKind::DataCell)
		{
			add(AccessiblePattern::Value); add(AccessiblePattern::Invoke);
			if (cell.m_Checked) add(AccessiblePattern::Toggle);
		}
		if (view.m_Impl->m_Kind == ListView::ViewKind::Tree)
		{
			state.m_Leaf = view.GetModel()->GetRowCount(node.m_Item) == 0;
			state.m_Expanded = view.IsExpanded(node.m_Item);
			if (!state.m_Leaf) add(AccessiblePattern::ExpandCollapse);
		}
		return state;
	}
	if (node.m_Kind == AccessibleKind::HeaderRow || node.m_Kind == AccessibleKind::HeaderCell)
	{
		auto& view = static_cast<ListView&>(widget);
		state.m_Info.m_Role = node.m_Kind == AccessibleKind::HeaderRow ? Role::Header : Role::HeaderItem;
		state.m_Info.m_Name = node.m_Kind == AccessibleKind::HeaderRow ? "Columns" : view.GetModel()->GetHeader(node.m_Index);
		const auto bounds = view.GetBounds();
		const Rect local = node.m_Kind == AccessibleKind::HeaderRow ? Rect{0, 0, view.GetActualSize().m_Width, view.HeaderHeight()} :
			Rect{2 + view.ColumnLeft(node.m_Index) - view.GetOffset().m_X, 0, view.GetColumnWidthImpl(node.m_Index), view.HeaderHeight()};
		state.m_Bounds = Clip(view, {bounds.m_X + local.m_X, bounds.m_Y, local.m_Width, local.m_Height}).Intersect(view.GetBounds());
		state.m_Visible = !state.m_Bounds.Empty(); state.m_Focusable = state.m_Focused = false;
		if (node.m_Kind == AccessibleKind::HeaderCell) add(AccessiblePattern::Invoke);
		return state;
	}
	if (node.m_Kind == AccessibleKind::RadioGroup)
	{
		auto& radio = static_cast<RadioButton&>(widget);
		state.m_Info = {}; state.m_Info.m_Role = Role::Group; state.m_Info.m_IsContentElement = true;
		state.m_Info.m_Name = radio.GetGroup() == "Default" ? std::string{} : std::string(radio.GetGroup());
		state.m_Focusable = state.m_Focused = false;
		state.m_Bounds = {};
		for (std::size_t index = 0, count = ChildCount(node); index < count; ++index)
		{
			const auto rect = Describe(*Child(node, index)).m_Bounds;
			if (rect.Empty()) continue;
			if (state.m_Bounds.Empty()) state.m_Bounds = rect;
			else { const float x = std::min(rect.m_X, state.m_Bounds.m_X), y = std::min(rect.m_Y, state.m_Bounds.m_Y); state.m_Bounds = {x, y, std::max(rect.Right(), state.m_Bounds.Right()) - x, std::max(rect.Bottom(), state.m_Bounds.Bottom()) - y}; }
		}
		add(AccessiblePattern::Selection); return state;
	}
	if (node.m_Kind == AccessibleKind::TabItem)
	{
		auto& tabs = static_cast<TabView&>(widget);
		state.m_Info.m_Role = Role::TabItem; state.m_Info.m_Name = tabs.m_Titles[node.m_Index];
		state.m_Selected = tabs.GetSelectedIndex() == node.m_Index; state.m_Focused = tabs.HasFocus() && state.m_Selected;
		const auto headers = tabs.Headers();
		if (node.m_Index < headers.size()) { auto rect = headers[node.m_Index]; const auto origin = tabs.GetBounds(); rect.m_X += origin.m_X; rect.m_Y += origin.m_Y; state.m_Bounds = Clip(tabs, rect); }
		add(AccessiblePattern::SelectionItem); add(AccessiblePattern::ScrollItem); state.m_Focusable = true;
	}
	else if (node.m_Kind == AccessibleKind::ComboItem)
	{
		auto& combo = static_cast<ComboBox&>(widget);
		state.m_Info.m_Role = Role::ListItem; state.m_Info.m_Name = combo.GetItems()[node.m_Index];
		state.m_Selected = combo.GetSelectedIndex() == static_cast<int>(node.m_Index); state.m_Focusable = true;
		state.m_Focused = combo.HasFocus() && combo.IsExpanded() && combo.m_Highlight == static_cast<int>(node.m_Index);
		state.m_Bounds = combo.IsExpanded() ? Rect{combo.m_PopupBounds.m_X + 4, combo.m_PopupBounds.m_Y + 4 + (static_cast<float>(node.m_Index) - static_cast<float>(combo.m_TopIndex)) * combo.m_RowHeight, combo.m_PopupBounds.m_Width - 8, combo.m_RowHeight}.Intersect(combo.VisiblePopup()) : Rect{};
		add(AccessiblePattern::SelectionItem); add(AccessiblePattern::ScrollItem);
	}
	else if (node.m_Kind == AccessibleKind::MenuItem)
	{
		auto& menu = static_cast<ContextMenu&>(widget);
		const auto& item = menu.GetItems()[node.m_Index];
		state.m_Info.m_Role = item.m_Separator ? Role::Separator : Role::MenuItem;
		state.m_Info.m_Name = item.m_Command ? item.m_Command->GetText() : item.m_Text;
		state.m_Enabled = state.m_Enabled && menu.Selectable(node.m_Index); state.m_Focusable = !item.m_Separator;
		state.m_Focused = menu.HasFocus() && menu.GetCurrentItem() == node.m_Index;
		state.m_Checked = item.m_Command && item.m_Command->IsChecked();
		state.m_Leaf = item.m_Children.empty(); state.m_Expanded = ChildCount(node) != 0;
		const auto bounds = menu.GetBounds();
		state.m_Bounds = Clip(menu, {bounds.m_X + 4, bounds.m_Y + menu.RowTop(node.m_Index), std::max(0.0f, bounds.m_Width - 8), menu.RowHeight(node.m_Index)}).Intersect(bounds);
		if (!item.m_Separator) { add(AccessiblePattern::Invoke); if (!state.m_Leaf) add(AccessiblePattern::ExpandCollapse); }
		if (item.m_Command) { state.m_Accelerator = Gesture(item.m_Command->GetShortcut()); if (item.m_Command->IsCheckable()) add(AccessiblePattern::Toggle); }
	}
	else if (node.m_Kind == AccessibleKind::MenuHeader)
	{
		auto& bar = static_cast<MenuBar&>(widget);
		state.m_Info.m_Role = Role::MenuItem; state.m_Info.m_Name = Caption(bar.GetItems()[node.m_Index].m_Text);
		state.m_Focusable = true; state.m_Focused = bar.HasFocus() && bar.m_Current == node.m_Index;
		state.m_Leaf = false; state.m_Expanded = ChildCount(node) != 0;
		if (node.m_Index < bar.m_Headers.size()) { auto rect = bar.m_Headers[node.m_Index]; const auto origin = bar.GetBounds(); rect.m_X += origin.m_X; rect.m_Y += origin.m_Y; state.m_Bounds = Clip(bar, rect); }
		const auto& text = bar.GetItems()[node.m_Index].m_Text;
		for (std::size_t index = 0; index + 1 < text.size(); ++index) if (text[index] == '&') { if (text[index + 1] == '&') { ++index; continue; } state.m_AccessKey = "Alt+" + text.substr(index + 1, 1); break; }
		add(AccessiblePattern::Invoke); add(AccessiblePattern::ExpandCollapse);
	}
	else if (node.m_Kind == AccessibleKind::SplitGrip)
	{
		state.m_Info.m_Role = Role::Separator; state.m_Info.m_Name = "Split position"; state.m_Focusable = true;
		auto rect = widget.GetFocusBounds(); const auto bounds = widget.GetBounds(); rect.m_X += bounds.m_X; rect.m_Y += bounds.m_Y;
		state.m_Bounds = Clip(widget, rect); add(AccessiblePattern::RangeValue);
	}
	else
	{
		Role role = Role::Group;
		if (dynamic_cast<RadioButton*>(&widget)) { role = Role::RadioButton; add(AccessiblePattern::SelectionItem); add(AccessiblePattern::Invoke); state.m_Selected = static_cast<RadioButton&>(widget).IsChecked(); }
		else if (auto* check = dynamic_cast<CheckBox*>(&widget)) { role = dynamic_cast<ToggleSwitch*>(check) ? Role::Switch : Role::CheckBox; state.m_Checked = check->IsChecked(); add(AccessiblePattern::Toggle); add(AccessiblePattern::Invoke); }
		else if (auto* button = dynamic_cast<Button*>(&widget))
		{
			role = Role::Button; add(AccessiblePattern::Invoke);
			if (const auto command = button->GetCommand(); command) { state.m_Accelerator = Gesture(command->GetShortcut()); if (command->IsCheckable()) { add(AccessiblePattern::Toggle); state.m_Checked = command->IsChecked(); } }
		}
		else if (auto* text = dynamic_cast<TextBox*>(&widget))
		{
			role = Role::Edit; state.m_Password = text->IsPassword(); state.m_ReadOnly = text->IsReadOnly();
			add(AccessiblePattern::Value); if (!state.m_Password) add(AccessiblePattern::Text);
			if (text->m_Impl->m_Multiline) add(AccessiblePattern::Scroll);
			if (state.m_Info.m_Name.empty()) state.m_Info.m_Name = text->m_Impl->m_Placeholder;
		}
		else if (auto* number = dynamic_cast<NumberBox*>(&widget)) { role = Role::NumberBox; state.m_ReadOnly = number->IsReadOnly(); state.m_Value = number->GetEditor().GetAccessibleText(); add(AccessiblePattern::Value); add(AccessiblePattern::RangeValue); }
		else if (dynamic_cast<Slider*>(&widget)) { role = Role::Slider; add(AccessiblePattern::RangeValue); }
		else if (auto* progress = dynamic_cast<ProgressBar*>(&widget)) { role = Role::ProgressBar; state.m_ReadOnly = true; if (!progress->IsIndeterminate()) add(AccessiblePattern::RangeValue); else state.m_ItemStatus = "Busy"; }
		else if (auto* combo = dynamic_cast<ComboBox*>(&widget)) { role = Role::ComboBox; state.m_Value = combo->GetSelectedText(); state.m_ReadOnly = true; state.m_Expanded = combo->IsExpanded(); state.m_Leaf = combo->GetItems().empty(); add(AccessiblePattern::Selection); add(AccessiblePattern::ExpandCollapse); add(AccessiblePattern::Value); }
		else if (dynamic_cast<TabView*>(&widget)) { role = Role::Tab; add(AccessiblePattern::Selection); }
		else if (dynamic_cast<ContextMenu*>(&widget)) role = Role::Menu;
		else if (dynamic_cast<MenuBar*>(&widget)) role = Role::MenuBar;
		else if (auto* view = dynamic_cast<ListView*>(&widget))
		{
			role = view->m_Impl->m_Kind == ListView::ViewKind::Tree ? Role::Tree : view->m_Impl->m_Kind == ListView::ViewKind::Table ? Role::Table : Role::List;
			add(AccessiblePattern::Selection); add(AccessiblePattern::Scroll); add(AccessiblePattern::ItemContainer);
			if (role == Role::Table) { add(AccessiblePattern::Grid); add(AccessiblePattern::Table); }
		}
		else if (dynamic_cast<ScrollView*>(&widget)) { role = Role::ScrollView; add(AccessiblePattern::Scroll); }
		else if (auto* dialog = dynamic_cast<Dialog*>(&widget)) { role = Role::Dialog; state.m_Modal = true; add(AccessiblePattern::Window); if (state.m_Info.m_Name.empty()) state.m_Info.m_Name = dialog->m_Title->GetText(); }
		else if (auto* tooltip = dynamic_cast<ToolTip*>(&widget)) { role = Role::ToolTip; if (state.m_Info.m_Name.empty() && !tooltip->GetChildren().empty()) state.m_Info.m_Name = PlainName(*tooltip->GetChildren().front()); }
		else if (dynamic_cast<ToolBar*>(&widget)) role = Role::ToolBar;
		else if (dynamic_cast<StatusBar*>(&widget)) role = Role::StatusBar;
		else if (dynamic_cast<Separator*>(&widget)) role = Role::Separator;
		else if (dynamic_cast<Label*>(&widget)) role = Role::Text;
		else if (dynamic_cast<ImageView*>(&widget)) role = Role::Image;
		if (state.m_Info.m_Role == Role::Automatic) state.m_Info.m_Role = role;
		if (state.m_Info.m_Name.empty()) state.m_Info.m_Name = PlainName(widget);
		if (auto* parent = dynamic_cast<NumberBox*>(widget.GetParent()); parent && state.m_Info.m_Name.empty()) state.m_Info.m_Name = Describe(ForWidget(*parent)).m_Info.m_Name;
		if (dynamic_cast<NumberBox*>(widget.GetParent()) && dynamic_cast<Button*>(&widget) && !widget.GetToolTip().empty()) state.m_Info.m_Name = widget.GetToolTip();
		if (state.m_Info.m_Role == Role::Button) add(AccessiblePattern::Invoke);
	}
	state.m_Visible = state.m_Visible && !state.m_Bounds.Empty();
	if (node.m_Kind == AccessibleKind::Widget) { const auto focus = Focus(); state.m_Focused = focus && *focus == node; }
	if (!state.m_Info.m_IsContentElement) state.m_Info.m_IsContentElement = state.m_Info.m_Role != Role::Group && state.m_Info.m_Role != Role::Separator;
	if (state.m_Info.m_AutomationId.empty()) state.m_Info.m_AutomationId = "Yukiri." + std::to_string(node.m_WidgetId);
	return state;
}
bool AccessibilityTree::HasPattern(const AccessibleNode& node, AccessiblePattern pattern) const { return IsAvailable(node) && (Describe(node).m_Patterns & static_cast<std::uint64_t>(pattern)) != 0; }
}
