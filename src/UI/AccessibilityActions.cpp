#include "AccessibilityTree.hpp"
#include "WidgetTree.hpp"
#include "Controls/ItemViewState.hpp"
#include "Controls/TextBoxState.hpp"
#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Controls/ContextMenu.hpp>
#include <Yukiri/Controls/Dialog.hpp>
#include <Yukiri/Controls/MenuBar.hpp>
#include <Yukiri/Controls/NumberBox.hpp>
#include <Yukiri/Controls/ProgressBar.hpp>
#include <Yukiri/Controls/RadioButton.hpp>
#include <Yukiri/Controls/Slider.hpp>
#include <Yukiri/Controls/ScrollView.hpp>
#include <Yukiri/Controls/TabView.hpp>
#include <Yukiri/Layout/SplitPanel.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <cmath>
#include <limits>

namespace Yukiri::Detail
{
namespace
{
Result<void> Unsupported() { return std::unexpected(Error{ErrorCode::InvalidArgument, "The accessibility operation is not supported by this element."}); }
}
std::optional<AccessibleNode> AccessibilityTree::Focus() const
{
	if (m_Lifetime.expired() || !m_Tree->m_Focus) return {};
	auto& focus = *m_Tree->m_Focus;
	AccessibleNode node = ForWidget(focus);
	if (auto* view = dynamic_cast<ListView*>(&focus); view && view->GetSelectionModel()->GetCurrent())
		node = ForItem(*view, view->GetSelectionModel()->GetCurrent(), view->m_Impl->m_Kind == ListView::ViewKind::Table ? std::optional(view->GetCurrentColumn()) : std::nullopt);
	else if (auto* tabs = dynamic_cast<TabView*>(&focus); tabs && !tabs->m_Titles.empty()) node = Part(AccessibleKind::TabItem, focus, tabs->GetSelectedIndex());
	else if (auto* combo = dynamic_cast<ComboBox*>(&focus); combo && combo->IsExpanded()) node = Part(AccessibleKind::ComboItem, focus, static_cast<std::size_t>(combo->m_Highlight));
	else if (auto* menu = dynamic_cast<ContextMenu*>(&focus); menu && menu->GetCurrentItem()) node = Part(AccessibleKind::MenuItem, focus, *menu->GetCurrentItem());
	else if (auto* bar = dynamic_cast<MenuBar*>(&focus); bar && !bar->GetItems().empty()) node = Part(AccessibleKind::MenuHeader, focus, bar->m_Current);
	else if (dynamic_cast<SplitPanel*>(&focus)) node = Part(AccessibleKind::SplitGrip, focus);
	return IsAvailable(node) ? std::optional(node) : std::nullopt;
}
std::optional<AccessibleNode> AccessibilityTree::AtPoint(Point point) const
{
	if (m_Lifetime.expired()) return {};
	if (m_Tree->m_Window && m_Tree->m_Window->GetConfig().m_Decoration == WindowDecoration::Custom && point.m_Y < 40)
	{
		auto title = Root(); title.m_Kind = AccessibleKind::TitleBar;
		for (std::size_t index = 0; index < 3; ++index) { auto button = *Child(title, index); if (Describe(button).m_Bounds.Contains(point)) return button; }
		return title;
	}
	for (const auto& popup : m_Tree->m_Popups) if (!popup->m_Closing && !popup->m_Content)
		if (auto* combo = dynamic_cast<ComboBox*>(popup->m_Owner); combo && combo->VisiblePopup().Contains(point))
		{
			const int index = static_cast<int>(std::floor((point.m_Y - combo->m_PopupBounds.m_Y - 4) / combo->m_RowHeight)) + combo->m_TopIndex;
			if (index >= 0 && static_cast<std::size_t>(index) < combo->GetItems().size()) return Part(AccessibleKind::ComboItem, *combo, static_cast<std::size_t>(index));
		}
	auto* widget = m_Tree->HitTarget(point);
	if (!widget) return {};
	if (auto* view = dynamic_cast<ListView*>(widget))
	{
		const auto local = view->ToLocal(point);
		const auto column = view->ColumnAt(local.m_X);
		if (view->m_Impl->m_Kind == ListView::ViewKind::Table && local.m_Y < view->HeaderHeight())
			return Part(column ? AccessibleKind::HeaderCell : AccessibleKind::HeaderRow, *view, column.value_or(0));
		if (const auto row = view->RowAt(local); row && column) return ForItem(*view, view->GetItemAt(*row), view->m_Impl->m_Kind == ListView::ViewKind::Table ? column : std::nullopt);
	}
	if (auto* tabs = dynamic_cast<TabView*>(widget))
	{
		const auto headers = tabs->Headers();
		for (std::size_t index = 0; index < headers.size(); ++index) if (headers[index].Contains(tabs->ToLocal(point))) return Part(AccessibleKind::TabItem, *tabs, index);
	}
	if (auto* menu = dynamic_cast<ContextMenu*>(widget))
	{
		const float y = menu->ToLocal(point).m_Y;
		for (std::size_t index = 0; index < menu->GetItems().size(); ++index) if (y >= menu->RowTop(index) && y < menu->RowTop(index) + menu->RowHeight(index)) return Part(AccessibleKind::MenuItem, *menu, index);
	}
	if (auto* bar = dynamic_cast<MenuBar*>(widget)) for (std::size_t index = 0; index < bar->m_Headers.size(); ++index)
		if (bar->m_Headers[index].Contains(bar->ToLocal(point))) return Part(AccessibleKind::MenuHeader, *bar, index);
	if (dynamic_cast<SplitPanel*>(widget) && widget->GetFocusBounds().Contains(widget->ToLocal(point))) return Part(AccessibleKind::SplitGrip, *widget);
	const auto node = ForWidget(*widget);
	return IsAvailable(node) ? std::optional(node) : std::nullopt;
}
Result<void> AccessibilityTree::SetFocus(const AccessibleNode& node)
{
	if (!IsAvailable(node) || !Describe(node).m_Enabled) return Unsupported();
	for (auto* ancestor = node.m_Owner; ancestor; ancestor = ancestor->GetParent()) if (!ancestor->IsVisible()) return Unsupported();
	Result<void> result;
	Dispatch([&]
	{
		if (!node.m_Owner) { if (m_Tree->m_Window) result = m_Tree->m_Window->Activate(); return; }
		if (node.m_Kind == AccessibleKind::DataRow || node.m_Kind == AccessibleKind::DataCell)
		{
			auto& view = static_cast<ListView&>(*node.m_Owner);
			view.GetSelectionModel()->SetCurrent(node.m_Item);
			if (node.m_Kind == AccessibleKind::DataCell) result = view.SetCurrentColumn(node.m_Index);
			view.ScrollIntoView(node.m_Item); view.RequestFocus();
		}
		else if (node.m_Kind == AccessibleKind::TabItem) { auto& tabs = static_cast<TabView&>(*node.m_Owner); result = tabs.SetSelectedIndex(node.m_Index); tabs.RequestFocus(); }
		else if (node.m_Kind == AccessibleKind::ComboItem)
		{
			auto& combo = static_cast<ComboBox&>(*node.m_Owner);
			combo.RequestFocus(); combo.SetExpanded(true); combo.m_Highlight = static_cast<int>(node.m_Index);
			combo.m_TopIndex = std::clamp(combo.m_TopIndex, std::max(0, combo.m_Highlight - combo.m_VisibleRows + 1), combo.m_Highlight); combo.RequestAnimationFrame();
		}
		else if (node.m_Kind == AccessibleKind::MenuItem) { auto& menu = static_cast<ContextMenu&>(*node.m_Owner); menu.Select(node.m_Index, true); menu.RequestFocus(); }
		else if (node.m_Kind == AccessibleKind::MenuHeader) { auto& menu = static_cast<MenuBar&>(*node.m_Owner); menu.m_Current = node.m_Index; menu.RequestFocus(); menu.InvalidatePaint(); }
		else if (node.m_Kind == AccessibleKind::RadioGroup) result = Unsupported();
		else node.m_Owner->RequestFocus();
	});
	return result;
}
Result<void> AccessibilityTree::Invoke(const AccessibleNode& node)
{
	if (!HasPattern(node, AccessiblePattern::Invoke) || !Describe(node).m_Enabled) return Unsupported();
	Result<void> result;
	Dispatch([&]
	{
		if (node.m_Kind == AccessibleKind::CaptionButton) { result = WindowAction(node, static_cast<int>(node.m_Index)); return; }
		if (node.m_Kind == AccessibleKind::MenuItem) { auto& menu = static_cast<ContextMenu&>(*node.m_Owner); menu.Select(node.m_Index, true); menu.InvokeCurrent(); }
		else if (node.m_Kind == AccessibleKind::MenuHeader) static_cast<MenuBar&>(*node.m_Owner).Open(node.m_Index);
		else if (node.m_Kind == AccessibleKind::HeaderCell)
		{
			auto& view = static_cast<ListView&>(*node.m_Owner); const auto sorting = view.GetModel()->GetSort();
			result = view.SortImpl(node.m_Index, !sorting || sorting->first != node.m_Index || !sorting->second);
		}
		else if (node.m_Kind == AccessibleKind::DataRow || node.m_Kind == AccessibleKind::DataCell)
		{
			auto& view = static_cast<ListView&>(*node.m_Owner);
			if (!Describe(node).m_ReadOnly) result = view.BeginEdit(node.m_Item, node.m_Index);
			else view.m_ItemActivated.Emit(node.m_Item);
		}
		else { KeyEvent event{Key::Enter, {}}; node.m_Owner->OnKeyDown(event); }
	});
	return result;
}
std::vector<AccessibleNode> AccessibilityTree::Selection(const AccessibleNode& node) const
{
	std::vector<AccessibleNode> result;
	if (!HasPattern(node, AccessiblePattern::Selection)) return result;
	if (auto* view = dynamic_cast<ListView*>(node.m_Owner))
	{
		for (const auto item : view->GetSelectionModel()->GetSelected()) { auto selected = ForItem(*view, item); if (IsAvailable(selected)) result.push_back(std::move(selected)); }
	}
	else if (node.m_Kind == AccessibleKind::RadioGroup)
	{
		for (std::size_t index = 0, count = ChildCount(node); index < count; ++index) { const auto child = *Child(node, index); if (Describe(child).m_Selected) result.push_back(child); }
	}
	else if (auto* combo = dynamic_cast<ComboBox*>(node.m_Owner); combo && combo->GetSelectedIndex() >= 0) result.push_back(Part(AccessibleKind::ComboItem, *combo, static_cast<std::size_t>(combo->GetSelectedIndex())));
	else if (auto* tabs = dynamic_cast<TabView*>(node.m_Owner); tabs && !tabs->m_Titles.empty()) result.push_back(Part(AccessibleKind::TabItem, *tabs, tabs->GetSelectedIndex()));
	return result;
}
Result<void> AccessibilityTree::Toggle(const AccessibleNode& node)
{
	if (!HasPattern(node, AccessiblePattern::Toggle) || !Describe(node).m_Enabled) return Unsupported();
	if (node.m_Kind == AccessibleKind::DataCell || node.m_Kind == AccessibleKind::DataRow)
	{
		Result<void> result;
		Dispatch([&]
		{
			auto& view = static_cast<ListView&>(*node.m_Owner);
			const auto cell = view.GetModel()->GetCell(node.m_Item, node.m_Index);
			result = view.GetModel()->SetChecked(node.m_Item, node.m_Index, !cell.m_Checked.value_or(false));
		});
		return result;
	}
	return Invoke(node);
}
bool AccessibilityTree::CanSelectMultiple(const AccessibleNode& node) const
{
	const auto* view = dynamic_cast<ListView*>(node.m_Owner);
	return node.m_Kind == AccessibleKind::Widget && view && view->GetSelectionModel()->GetMode() == SelectionModel::Mode::Multiple;
}
std::uint64_t AccessibilityTree::SelectionVersion(const AccessibleNode& node) const
{
	if (!IsAvailable(node)) return 0;
	if (auto* view = dynamic_cast<ListView*>(node.m_Owner)) return view->GetSelectionModel()->GetVersion();
	if (auto* combo = dynamic_cast<ComboBox*>(node.m_Owner)) return static_cast<std::uint64_t>(combo->GetSelectedIndex() + 1) ^ (combo->m_ItemsGeneration << 32);
	if (auto* tabs = dynamic_cast<TabView*>(node.m_Owner)) return tabs->GetSelectedIndex();
	if (node.m_Kind == AccessibleKind::RadioGroup)
	{
		for (std::size_t index = 0, count = ChildCount(node); index < count; ++index) { const auto child = Child(node, index); if (child && static_cast<RadioButton&>(*child->m_Owner).IsChecked()) return child->m_WidgetId; }
	}
	return 0;
}
Result<void> AccessibilityTree::Select(const AccessibleNode& node, bool add, bool remove)
{
	if (!HasPattern(node, AccessiblePattern::SelectionItem) || !Describe(node).m_Enabled) return Unsupported();
	Result<void> result;
	Dispatch([&]
	{
		if (node.m_Kind == AccessibleKind::DataRow || node.m_Kind == AccessibleKind::DataCell)
		{
			auto& view = static_cast<ListView&>(*node.m_Owner);
			if (remove) { auto selected = view.GetSelectionModel()->GetSelected(); std::erase(selected, node.m_Item); view.GetSelectionModel()->SetSelected(std::move(selected), view.GetSelectionModel()->GetCurrent()); }
			else view.GetSelectionModel()->Select(node.m_Item, add);
			if (node.m_Kind == AccessibleKind::DataCell) result = view.SetCurrentColumn(node.m_Index);
			view.ScrollIntoView(node.m_Item);
		}
		else if (remove)
		{
			if (node.m_Kind == AccessibleKind::ComboItem) { auto& combo = static_cast<ComboBox&>(*node.m_Owner); if (combo.GetSelectedIndex() == static_cast<int>(node.m_Index)) result = combo.SetSelectedIndex(-1); }
			else if (auto* radio = dynamic_cast<RadioButton*>(node.m_Owner)) radio->SetChecked(false);
			else result = Unsupported();
		}
		else if (node.m_Kind == AccessibleKind::TabItem) result = static_cast<TabView&>(*node.m_Owner).SetSelectedIndex(node.m_Index);
		else if (node.m_Kind == AccessibleKind::ComboItem) { auto& combo = static_cast<ComboBox&>(*node.m_Owner); result = combo.SetSelectedIndex(static_cast<int>(node.m_Index)); combo.SetExpanded(false); }
		else if (auto* radio = dynamic_cast<RadioButton*>(node.m_Owner)) { if (!radio->IsChecked()) { KeyEvent event{Key::Enter, {}}; radio->OnKeyDown(event); } }
	});
	return result;
}
Result<void> AccessibilityTree::SetExpanded(const AccessibleNode& node, bool expanded)
{
	if (!HasPattern(node, AccessiblePattern::ExpandCollapse) || !Describe(node).m_Enabled) return Unsupported();
	Dispatch([&]
	{
		if (node.m_Kind == AccessibleKind::DataRow) static_cast<ListView&>(*node.m_Owner).SetExpanded(node.m_Item, expanded);
		else if (node.m_Kind == AccessibleKind::MenuHeader) { auto& bar = static_cast<MenuBar&>(*node.m_Owner); if (expanded) bar.Open(node.m_Index); else bar.ClosePopup(); }
		else if (node.m_Kind == AccessibleKind::MenuItem) { auto& menu = static_cast<ContextMenu&>(*node.m_Owner); menu.Select(node.m_Index, true); if (expanded) menu.OpenSubmenu(); else if (menu.m_Child && !menu.m_ChildLifetime.expired()) menu.m_Child->ClosePopup(); }
		else if (auto* combo = dynamic_cast<ComboBox*>(node.m_Owner)) combo->SetExpanded(expanded);
	});
	return {};
}
std::optional<AccessibleRange> AccessibilityTree::Range(const AccessibleNode& node) const
{
	if (!HasPattern(node, AccessiblePattern::RangeValue)) return {};
	if (auto* slider = dynamic_cast<Slider*>(node.m_Owner)) return AccessibleRange{slider->GetValue(), slider->GetMinimum(), slider->GetMaximum(), slider->GetStep(), static_cast<double>(slider->GetStep()) * 10, false};
	if (auto* number = dynamic_cast<NumberBox*>(node.m_Owner)) return AccessibleRange{number->GetValue(), number->GetMinimum(), number->GetMaximum(), number->GetStep(), std::min(number->GetStep(), std::numeric_limits<double>::max() / 10) * 10, number->IsReadOnly()};
	if (auto* progress = dynamic_cast<ProgressBar*>(node.m_Owner)) return AccessibleRange{progress->GetValue() * 100, 0, 100, 0, 0, true};
	if (auto* split = dynamic_cast<SplitPanel*>(node.m_Owner)) return AccessibleRange{split->GetRatio(), 0, 1, 0.01, 0.1, false};
	return {};
}
Result<void> AccessibilityTree::SetRangeValue(const AccessibleNode& node, double value)
{
	const auto range = Range(node);
	if (!range || range->m_ReadOnly || !Describe(node).m_Enabled || !std::isfinite(value) || value < range->m_Minimum || value > range->m_Maximum) return Unsupported();
	Dispatch([&]
	{
		if (auto* slider = dynamic_cast<Slider*>(node.m_Owner)) slider->SetValue(static_cast<float>(value));
		else if (auto* number = dynamic_cast<NumberBox*>(node.m_Owner)) number->SetValue(value);
		else if (auto* split = dynamic_cast<SplitPanel*>(node.m_Owner)) split->SetRatio(static_cast<float>(value));
	});
	return {};
}
std::string AccessibilityTree::Value(const AccessibleNode& node) const
{
	if (!IsAvailable(node)) return {};
	if (auto* text = dynamic_cast<TextBox*>(node.m_Owner)) return text->GetAccessibleText();
	return Describe(node).m_Value;
}
std::uint64_t AccessibilityTree::TextVersion(const AccessibleNode& node) const
{
	if (!IsAvailable(node)) return 0;
	if (auto* text = dynamic_cast<TextBox*>(node.m_Owner)) return text->m_Impl->m_Editor.m_Revision;
	return 0;
}
TextBox* AccessibilityTree::TextControl(const AccessibleNode& node) const
{
	if (!IsAvailable(node) || node.m_Kind != AccessibleKind::Widget) return nullptr;
	auto* text = dynamic_cast<TextBox*>(node.m_Owner);
	return text && !text->IsPassword() ? text : nullptr;
}
Result<void> AccessibilityTree::SetValue(const AccessibleNode& node, std::string value)
{
	if (!HasPattern(node, AccessiblePattern::Value) || Describe(node).m_ReadOnly || !Describe(node).m_Enabled) return Unsupported();
	Result<void> result;
	Dispatch([&]
	{
		if (node.m_Kind == AccessibleKind::DataCell || node.m_Kind == AccessibleKind::DataRow) { auto& view = static_cast<ListView&>(*node.m_Owner); result = view.GetModel()->SetText(node.m_Item, node.m_Index, std::move(value)); }
		else if (auto* text = dynamic_cast<TextBox*>(node.m_Owner)) text->SetText(value);
		else if (auto* number = dynamic_cast<NumberBox*>(node.m_Owner)) { number->GetEditor().SetText(value); if (!number->Commit()) result = Unsupported(); }
	});
	return result;
}
std::optional<AccessibleScroll> AccessibilityTree::ScrollInfo(const AccessibleNode& node) const
{
	if (!HasPattern(node, AccessiblePattern::Scroll)) return {};
	if (auto* view = dynamic_cast<ListView*>(node.m_Owner)) return AccessibleScroll{view->GetOffset(), {view->ContentWidth(), static_cast<float>(view->GetRowCount()) * view->GetRowHeight()}, {view->m_Impl->m_Viewport.m_Width, view->m_Impl->m_Viewport.m_Height}};
	if (auto* scroll = dynamic_cast<ScrollView*>(node.m_Owner)) return AccessibleScroll{scroll->GetOffset(), {scroll->m_Horizontal ? scroll->m_Content.m_Width : scroll->m_Viewport.m_Width, scroll->m_Vertical ? scroll->m_Content.m_Height : scroll->m_Viewport.m_Height}, scroll->m_Viewport};
	if (auto* text = dynamic_cast<TextBox*>(node.m_Owner)) { text->PrepareLayout(); return AccessibleScroll{text->GetOffset(), text->m_Impl->m_Document.GetSize(), {text->m_Impl->m_Viewport.m_Width, text->m_Impl->m_Viewport.m_Height}}; }
	return {};
}
Result<void> AccessibilityTree::SetScroll(const AccessibleNode& node, Point offset)
{
	if (!HasPattern(node, AccessiblePattern::Scroll) || !Describe(node).m_Enabled || !std::isfinite(offset.m_X) || !std::isfinite(offset.m_Y)) return Unsupported();
	Dispatch([&]
	{
		if (auto* view = dynamic_cast<ListView*>(node.m_Owner)) view->SetOffset(offset);
		else if (auto* scroll = dynamic_cast<ScrollView*>(node.m_Owner)) scroll->SetOffset(offset);
		else if (auto* text = dynamic_cast<TextBox*>(node.m_Owner)) text->SetOffset(offset);
	});
	return {};
}
Result<void> AccessibilityTree::Realize(const AccessibleNode& node)
{
	if (!IsAvailable(node)) return Unsupported();
	Dispatch([&]
	{
		if (node.m_Kind == AccessibleKind::DataCell || node.m_Kind == AccessibleKind::DataRow)
		{
			auto& view = static_cast<ListView&>(*node.m_Owner); view.ScrollIntoView(node.m_Item);
			if (node.m_Kind == AccessibleKind::DataCell)
			{
				auto offset = view.GetOffset(); const float x = view.ColumnLeft(node.m_Index), width = view.GetColumnWidthImpl(node.m_Index), viewport = view.m_Impl->m_Viewport.m_Width;
				if (x < offset.m_X || width > viewport) offset.m_X = x;
				else if (x + width > offset.m_X + viewport) offset.m_X = x + width - viewport;
				view.SetOffset(offset);
			}
		}
		else if (node.m_Kind == AccessibleKind::ComboItem) { const auto focused = SetFocus(node); (void)focused; }
		else if (node.m_Owner) node.m_Owner->BringIntoView();
	});
	return {};
}
Result<void> AccessibilityTree::WindowAction(const AccessibleNode& node, int action)
{
	if (!IsAvailable(node)) return Unsupported();
	Result<void> result;
	Dispatch([&]
	{
		if (auto* dialog = dynamic_cast<Dialog*>(node.m_Owner)) { if (action == 2) dialog->Complete(Dialog::Result::Cancel); else result = Unsupported(); return; }
		auto* window = m_Tree->m_Window;
		if (!window) { result = Unsupported(); return; }
		if (action == 0) result = window->Minimize();
		else if (action == 1) result = window->IsMaximized() ? window->Restore() : window->Maximize();
		else if (action == 2) window->Close();
		else if (action == 3) result = window->Restore();
	});
	return result;
}
}
