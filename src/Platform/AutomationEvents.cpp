#include "AutomationProvider.hpp"
#include <Yukiri/Controls/TextBox.hpp>

namespace Yukiri::Detail
{
namespace
{
struct Variant
{
	VARIANT m_Value{};
	~Variant() { VariantClear(&m_Value); }
	Variant(const Variant&) = delete;
	Variant() = default;
};
}
HRESULT AutomationProvider::AdviseEventAdded(EVENTID id, SAFEARRAY* properties) { return Advise(id, properties, true); }
HRESULT AutomationProvider::AdviseEventRemoved(EVENTID id, SAFEARRAY* properties) { return Advise(id, properties, false); }
HRESULT AutomationProvider::Advise(EVENTID id, SAFEARRAY* properties, bool added)
{
	if (id != UIA_AutomationPropertyChangedEventId) return S_OK;
	auto update = [&]() -> HRESULT
	{
		std::vector<PROPERTYID> ids;
		if (properties)
		{
			VARTYPE type{}; SafeArrayGetVartype(properties, &type);
			if (SafeArrayGetDim(properties) != 1 || type != VT_I4) return E_INVALIDARG;
			LONG first = 0, last = -1; SafeArrayGetLBound(properties, 1, &first); SafeArrayGetUBound(properties, 1, &last);
			for (std::int64_t offset = first; offset <= last; ++offset) { LONG index = static_cast<LONG>(offset); PROPERTYID property = 0; const auto read = SafeArrayGetElement(properties, &index, &property); if (FAILED(read)) return read; ids.push_back(property); }
		}
		std::lock_guard lock(m_Context->m_Mutex);
		if (ids.empty())
		{
			if (added) { ++m_AllProperties; ++m_Context->m_AllProperties; }
			else if (m_AllProperties) { --m_AllProperties; --m_Context->m_AllProperties; }
		}
		for (const auto property : ids)
		{
			auto& local = m_Subscriptions[property]; auto& global = m_Context->m_Subscriptions[property];
			if (added) { ++local; ++global; } else if (local) { --local; if (global) --global; }
		}
		return S_OK;
	};
	if (added) return Read(update);
	try { return update(); }
	catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
	catch (...) { return E_FAIL; }
}
void AutomationProvider::Flush()
{
	const auto ignored = Read([&]() -> HRESULT
	{
		auto state = m_Context->m_Tree->Describe(m_Node);
		state.m_Enabled = state.m_Enabled && (!m_Context->m_Handle || IsWindowEnabled(m_Context->m_Handle));
		const auto range = m_Context->m_Tree->Range(m_Node);
		const auto scroll = m_Context->m_Tree->ScrollInfo(m_Node);
		const auto textVersion = m_Context->m_Tree->TextVersion(m_Node);
		const auto selectionVersion = m_Context->m_Tree->SelectionVersion(m_Node);
		const auto* text = m_Context->m_Tree->TextControl(m_Node);
		const auto textSelection = text ? text->GetSelection() : std::pair<std::size_t, std::size_t>{};
		UiaRect screen{}; get_BoundingRectangle(&screen);
		WindowVisualState windowState{}; const bool window = (state.m_Patterns & static_cast<std::uint64_t>(AccessiblePattern::Window)) != 0;
		if (window) get_WindowVisualState(&windowState);
		const bool observeValue = text && !state.m_Password && m_Context->IsPropertyObserved(UIA_ValueValuePropertyId);
		const auto value = observeValue && (textVersion != m_LastTextVersion || !m_RecordingTextValue) ? m_Context->m_Tree->Value(m_Node) : std::string{};
		if (m_LastState)
		{
			const auto& previous = *m_LastState;
			auto property = [&](PROPERTYID id, auto fill)
			{
				Variant before, after;
				fill(before.m_Value, after.m_Value);
				if (m_Context->m_Tree->IsAvailable(m_Node)) UiaRaiseAutomationPropertyChangedEvent(static_cast<IRawElementProviderSimple*>(this), id, before.m_Value, after.m_Value);
			};
			auto boolean = [&](PROPERTYID id, bool oldValue, bool newValue) { if (oldValue != newValue) property(id, [&](VARIANT& a, VARIANT& b) { a.vt = b.vt = VT_BOOL; a.boolVal = oldValue ? VARIANT_TRUE : VARIANT_FALSE; b.boolVal = newValue ? VARIANT_TRUE : VARIANT_FALSE; }); };
			auto integer = [&](PROPERTYID id, int oldValue, int newValue) { if (oldValue != newValue) property(id, [&](VARIANT& a, VARIANT& b) { a.vt = b.vt = VT_I4; a.lVal = oldValue; b.lVal = newValue; }); };
			auto number = [&](PROPERTYID id, double oldValue, double newValue) { if (oldValue != newValue) property(id, [&](VARIANT& a, VARIANT& b) { a.vt = b.vt = VT_R8; a.dblVal = oldValue; b.dblVal = newValue; }); };
			auto string = [&](PROPERTYID id, const std::string& oldValue, const std::string& newValue) { if (oldValue != newValue) property(id, [&](VARIANT& a, VARIANT& b) { a.vt = b.vt = VT_BSTR; StringValue(oldValue, &a.bstrVal); StringValue(newValue, &b.bstrVal); }); };
			string(UIA_NamePropertyId, previous.m_Info.m_Name, state.m_Info.m_Name);
			string(UIA_HelpTextPropertyId, previous.m_Info.m_Description, state.m_Info.m_Description);
			string(UIA_ItemStatusPropertyId, previous.m_ItemStatus, state.m_ItemStatus);
			boolean(UIA_IsEnabledPropertyId, previous.m_Enabled, state.m_Enabled);
			boolean(UIA_IsOffscreenPropertyId, !previous.m_Visible, !state.m_Visible);
			boolean(UIA_IsPasswordPropertyId, previous.m_Password, state.m_Password);
			const std::pair<PROPERTYID, AccessiblePattern> patterns[]{
				{UIA_IsTextPatternAvailablePropertyId, AccessiblePattern::Text}, {UIA_IsTextPattern2AvailablePropertyId, AccessiblePattern::Text},
				{UIA_IsRangeValuePatternAvailablePropertyId, AccessiblePattern::RangeValue}, {UIA_IsValuePatternAvailablePropertyId, AccessiblePattern::Value},
				{UIA_IsSelectionPatternAvailablePropertyId, AccessiblePattern::Selection}, {UIA_IsExpandCollapsePatternAvailablePropertyId, AccessiblePattern::ExpandCollapse}};
			for (const auto& [id, pattern] : patterns) { const auto flag = static_cast<std::uint64_t>(pattern); boolean(id, (previous.m_Patterns & flag) != 0, (state.m_Patterns & flag) != 0); }
			boolean(UIA_SelectionItemIsSelectedPropertyId, previous.m_Selected, state.m_Selected);
			boolean(UIA_ValueIsReadOnlyPropertyId, previous.m_ReadOnly, state.m_ReadOnly);
			integer(UIA_ToggleToggleStatePropertyId, previous.m_Checked ? ToggleState_On : ToggleState_Off, state.m_Checked ? ToggleState_On : ToggleState_Off);
			integer(UIA_ExpandCollapseExpandCollapseStatePropertyId, previous.m_Leaf ? ExpandCollapseState_LeafNode : previous.m_Expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed,
				state.m_Leaf ? ExpandCollapseState_LeafNode : state.m_Expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed);
			if (!state.m_Password) string(UIA_ValueValuePropertyId, previous.m_Value, state.m_Value);
			if (range && m_LastRange)
			{
				number(UIA_RangeValueValuePropertyId, m_LastRange->m_Value, range->m_Value);
				number(UIA_RangeValueMinimumPropertyId, m_LastRange->m_Minimum, range->m_Minimum);
				number(UIA_RangeValueMaximumPropertyId, m_LastRange->m_Maximum, range->m_Maximum);
			}
			if (scroll && m_LastScroll)
			{
				auto percent = [](float offset, float extent, float viewport) { return extent <= viewport ? ScrollNoChange : static_cast<double>(offset) / (extent - viewport) * 100; };
				number(UIA_ScrollHorizontalScrollPercentPropertyId, percent(m_LastScroll->m_Offset.m_X, m_LastScroll->m_Extent.m_Width, m_LastScroll->m_Viewport.m_Width), percent(scroll->m_Offset.m_X, scroll->m_Extent.m_Width, scroll->m_Viewport.m_Width));
				number(UIA_ScrollVerticalScrollPercentPropertyId, percent(m_LastScroll->m_Offset.m_Y, m_LastScroll->m_Extent.m_Height, m_LastScroll->m_Viewport.m_Height), percent(scroll->m_Offset.m_Y, scroll->m_Extent.m_Height, scroll->m_Viewport.m_Height));
			}
			if (screen.left != m_LastScreen.left || screen.top != m_LastScreen.top || screen.width != m_LastScreen.width || screen.height != m_LastScreen.height)
				property(UIA_BoundingRectanglePropertyId, [&](VARIANT& a, VARIANT& b)
				{
					a.vt = b.vt = VT_ARRAY | VT_R8;
					const double oldBounds[]{m_LastScreen.left, m_LastScreen.top, m_LastScreen.width, m_LastScreen.height}, newBounds[]{screen.left, screen.top, screen.width, screen.height};
					DoubleArray(oldBounds, &a.parray); DoubleArray(newBounds, &b.parray);
				});
			if (window) integer(UIA_WindowWindowVisualStatePropertyId, m_LastWindowState, windowState);
			if (text && textVersion != m_LastTextVersion)
			{
				UiaRaiseAutomationEvent(static_cast<IRawElementProviderSimple*>(this), UIA_Text_TextChangedEventId);
				if (observeValue && m_RecordingTextValue) string(UIA_ValueValuePropertyId, m_LastTextValue, value);
			}
			if (text && textSelection != m_LastTextSelection) UiaRaiseAutomationEvent(static_cast<IRawElementProviderSimple*>(this), UIA_Text_TextSelectionChangedEventId);
			if ((state.m_Patterns & static_cast<std::uint64_t>(AccessiblePattern::Selection)) && selectionVersion != m_LastSelectionVersion) UiaRaiseAutomationEvent(static_cast<IRawElementProviderSimple*>(this), UIA_Selection_InvalidatedEventId);
		}
		if (observeValue && (textVersion != m_LastTextVersion || !m_RecordingTextValue)) m_LastTextValue = value;
		if (!observeValue) m_LastTextValue.clear();
		m_RecordingTextValue = observeValue;
		m_LastState = state; m_LastRange = range; m_LastScroll = scroll; m_LastScreen = screen; m_LastWindowState = windowState;
		m_LastTextVersion = textVersion; m_LastTextSelection = textSelection; m_LastSelectionVersion = selectionVersion;
		return S_OK;
	});
	(void)ignored;
}
}
