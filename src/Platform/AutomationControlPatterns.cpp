#include "AutomationProvider.hpp"
#include <Yukiri/Controls/ListView.hpp>
#include <Yukiri/Controls/TextBox.hpp>
#include <array>
#include <cmath>
#include <limits>

namespace Yukiri::Detail
{
HRESULT AutomationProvider::Invoke() { return Action(AccessiblePattern::Invoke, [&] { return m_Context->m_Tree->Invoke(m_Node); }); }
HRESULT AutomationProvider::Toggle() { return Action(AccessiblePattern::Toggle, [&] { return m_Context->m_Tree->Toggle(m_Node); }); }
HRESULT AutomationProvider::get_ToggleState(ToggleState* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Toggle)) return UIA_E_NOTSUPPORTED; *result = m_Context->m_Tree->Describe(m_Node).m_Checked ? ToggleState_On : ToggleState_Off; return S_OK; });
}
HRESULT AutomationProvider::GetSelection(SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Selection)) return UIA_E_NOTSUPPORTED;
		return m_Context->NodesArray(m_Context->m_Tree->Selection(m_Node), result);
	});
}
HRESULT AutomationProvider::get_CanSelectMultiple(BOOL* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Selection)) return UIA_E_NOTSUPPORTED; *result = m_Context->m_Tree->CanSelectMultiple(m_Node); return S_OK; });
}
HRESULT AutomationProvider::get_IsSelectionRequired(BOOL* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Selection)) return UIA_E_NOTSUPPORTED; *result = m_Context->m_Tree->Describe(m_Node).m_Info.m_Role == AccessibilityInfo::Role::Tab; return S_OK; });
}
HRESULT AutomationProvider::Select() { return Action(AccessiblePattern::SelectionItem, [&] { return m_Context->m_Tree->Select(m_Node); }); }
HRESULT AutomationProvider::AddToSelection()
{
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::SelectionItem)) return UIA_E_NOTSUPPORTED;
		auto container = m_Node.m_Kind == AccessibleKind::DataRow || m_Node.m_Kind == AccessibleKind::DataCell ? std::optional(m_Context->m_Tree->ForWidget(*m_Node.m_Owner)) : m_Context->m_Tree->Parent(m_Node);
		if (container && !m_Context->m_Tree->CanSelectMultiple(*container))
		{
			const auto selected = m_Context->m_Tree->Selection(*container);
			if (!selected.empty() && !m_Context->m_Tree->Describe(m_Node).m_Selected) return UIA_E_INVALIDOPERATION;
		}
		return Action(AccessiblePattern::SelectionItem, [&] { return m_Context->m_Tree->Select(m_Node, true); });
	});
}
HRESULT AutomationProvider::RemoveFromSelection() { return Action(AccessiblePattern::SelectionItem, [&] { return m_Context->m_Tree->Select(m_Node, false, true); }); }
HRESULT AutomationProvider::get_IsSelected(BOOL* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::SelectionItem)) return UIA_E_NOTSUPPORTED; *result = m_Context->m_Tree->Describe(m_Node).m_Selected; return S_OK; });
}
HRESULT AutomationProvider::get_SelectionContainer(IRawElementProviderSimple** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::SelectionItem)) return UIA_E_NOTSUPPORTED;
		const auto container = m_Node.m_Kind == AccessibleKind::DataRow || m_Node.m_Kind == AccessibleKind::DataCell ? std::optional(m_Context->m_Tree->ForWidget(*m_Node.m_Owner)) : m_Context->m_Tree->Parent(m_Node);
		if (container) *result = m_Context->Get(*container).Detach();
		return S_OK;
	});
}
HRESULT AutomationProvider::Expand() { return Action(AccessiblePattern::ExpandCollapse, [&] { return m_Context->m_Tree->SetExpanded(m_Node, true); }); }
HRESULT AutomationProvider::Collapse() { return Action(AccessiblePattern::ExpandCollapse, [&] { return m_Context->m_Tree->SetExpanded(m_Node, false); }); }
HRESULT AutomationProvider::get_ExpandCollapseState(ExpandCollapseState* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::ExpandCollapse)) return UIA_E_NOTSUPPORTED; const auto state = m_Context->m_Tree->Describe(m_Node); *result = state.m_Leaf ? ExpandCollapseState_LeafNode : state.m_Expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed; return S_OK; });
}
HRESULT AutomationProvider::SetValue(LPCWSTR value)
{
	if (!value) return E_INVALIDARG;
	return Action(AccessiblePattern::Value, [&] { return m_Context->m_Tree->SetValue(m_Node, ToUtf8(value)); });
}
HRESULT AutomationProvider::get_Value(BSTR* result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Value)) return UIA_E_NOTSUPPORTED; if (m_Context->m_Tree->Describe(m_Node).m_Password) return E_ACCESSDENIED; return StringValue(m_Context->m_Tree->Value(m_Node), result); });
}
HRESULT AutomationProvider::SetValue(double value)
{
	if (!std::isfinite(value)) return E_INVALIDARG;
	return Read([&]() -> HRESULT { const auto range = m_Context->m_Tree->Range(m_Node); if (!range) return UIA_E_NOTSUPPORTED; if (value < range->m_Minimum || value > range->m_Maximum) return E_INVALIDARG; return Action(AccessiblePattern::RangeValue, [&] { return m_Context->m_Tree->SetRangeValue(m_Node, value); }); });
}
HRESULT AutomationProvider::get_IsReadOnly(BOOL* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Value) && !m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::RangeValue)) return UIA_E_NOTSUPPORTED; *result = m_Context->m_Tree->Describe(m_Node).m_ReadOnly; return S_OK; });
}
HRESULT AutomationProvider::RangeProperty(double* result, int property)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT
	{
		const auto range = m_Context->m_Tree->Range(m_Node); if (!range) return UIA_E_NOTSUPPORTED;
		*result = property == 0 ? range->m_Value : property == 1 ? range->m_Minimum : property == 2 ? range->m_Maximum : property == 3 ? range->m_SmallChange : range->m_LargeChange;
		return S_OK;
	});
}
HRESULT AutomationProvider::get_Value(double* result) { return RangeProperty(result, 0); }
HRESULT AutomationProvider::get_Minimum(double* result) { return RangeProperty(result, 1); }
HRESULT AutomationProvider::get_Maximum(double* result) { return RangeProperty(result, 2); }
HRESULT AutomationProvider::get_SmallChange(double* result) { return RangeProperty(result, 3); }
HRESULT AutomationProvider::get_LargeChange(double* result) { return RangeProperty(result, 4); }
HRESULT AutomationProvider::Scroll(ScrollAmount horizontal, ScrollAmount vertical)
{
	return Read([&]() -> HRESULT
	{
		const auto scroll = m_Context->m_Tree->ScrollInfo(m_Node); if (!scroll) return UIA_E_NOTSUPPORTED;
		auto amount = [](ScrollAmount value, float viewport) -> std::optional<float>
		{
			switch (value) { case ScrollAmount_NoAmount: return 0.0f; case ScrollAmount_SmallDecrement: return -32.0f; case ScrollAmount_SmallIncrement: return 32.0f; case ScrollAmount_LargeDecrement: return -viewport * 0.9f; case ScrollAmount_LargeIncrement: return viewport * 0.9f; default: return {}; }
		};
		const auto dx = amount(horizontal, scroll->m_Viewport.m_Width), dy = amount(vertical, scroll->m_Viewport.m_Height);
		if (!dx || !dy) return E_INVALIDARG;
		if ((*dx && scroll->m_Extent.m_Width <= scroll->m_Viewport.m_Width) || (*dy && scroll->m_Extent.m_Height <= scroll->m_Viewport.m_Height)) return UIA_E_INVALIDOPERATION;
		return Action(AccessiblePattern::Scroll, [&] { return m_Context->m_Tree->SetScroll(m_Node, {scroll->m_Offset.m_X + *dx, scroll->m_Offset.m_Y + *dy}); });
	});
}
HRESULT AutomationProvider::SetScrollPercent(double horizontal, double vertical)
{
	if (!std::isfinite(horizontal) || !std::isfinite(vertical) || (horizontal != ScrollNoChange && (horizontal < 0 || horizontal > 100)) || (vertical != ScrollNoChange && (vertical < 0 || vertical > 100))) return E_INVALIDARG;
	return Read([&]() -> HRESULT
	{
		const auto scroll = m_Context->m_Tree->ScrollInfo(m_Node); if (!scroll) return UIA_E_NOTSUPPORTED;
		if ((horizontal != ScrollNoChange && scroll->m_Extent.m_Width <= scroll->m_Viewport.m_Width) || (vertical != ScrollNoChange && scroll->m_Extent.m_Height <= scroll->m_Viewport.m_Height)) return UIA_E_INVALIDOPERATION;
		const auto x = horizontal == ScrollNoChange ? scroll->m_Offset.m_X : static_cast<float>(horizontal / 100) * (scroll->m_Extent.m_Width - scroll->m_Viewport.m_Width);
		const auto y = vertical == ScrollNoChange ? scroll->m_Offset.m_Y : static_cast<float>(vertical / 100) * (scroll->m_Extent.m_Height - scroll->m_Viewport.m_Height);
		return Action(AccessiblePattern::Scroll, [&] { return m_Context->m_Tree->SetScroll(m_Node, {x, y}); });
	});
}
HRESULT AutomationProvider::ScrollProperty(double* result, bool horizontal, bool viewSize)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT
	{
		const auto scroll = m_Context->m_Tree->ScrollInfo(m_Node); if (!scroll) return UIA_E_NOTSUPPORTED;
		const float extent = horizontal ? scroll->m_Extent.m_Width : scroll->m_Extent.m_Height;
		const float viewport = horizontal ? scroll->m_Viewport.m_Width : scroll->m_Viewport.m_Height;
		const float offset = horizontal ? scroll->m_Offset.m_X : scroll->m_Offset.m_Y;
		*result = viewSize ? extent <= 0 ? 100 : std::clamp(static_cast<double>(viewport) / extent * 100, 0.0, 100.0) : extent <= viewport ? ScrollNoChange : static_cast<double>(offset) / (extent - viewport) * 100;
		return S_OK;
	});
}
HRESULT AutomationProvider::get_HorizontalScrollPercent(double* result) { return ScrollProperty(result, true, false); }
HRESULT AutomationProvider::get_VerticalScrollPercent(double* result) { return ScrollProperty(result, false, false); }
HRESULT AutomationProvider::get_HorizontalViewSize(double* result) { return ScrollProperty(result, true, true); }
HRESULT AutomationProvider::get_VerticalViewSize(double* result) { return ScrollProperty(result, false, true); }
HRESULT AutomationProvider::get_HorizontallyScrollable(BOOL* result) { if (!result) return E_POINTER; double value = 0; const auto status = get_HorizontalScrollPercent(&value); if (SUCCEEDED(status)) *result = value != ScrollNoChange; return status; }
HRESULT AutomationProvider::get_VerticallyScrollable(BOOL* result) { if (!result) return E_POINTER; double value = 0; const auto status = get_VerticalScrollPercent(&value); if (SUCCEEDED(status)) *result = value != ScrollNoChange; return status; }
HRESULT AutomationProvider::ScrollIntoView() { return Action(AccessiblePattern::ScrollItem, [&] { return m_Context->m_Tree->Realize(m_Node); }); }
HRESULT AutomationProvider::Realize() { return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::VirtualizedItem)) return UIA_E_NOTSUPPORTED; return m_Context->m_Tree->Realize(m_Node) ? S_OK : UIA_E_INVALIDOPERATION; }); }
}
