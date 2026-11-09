#include "AutomationProvider.hpp"
#include <Yukiri/Controls/ListView.hpp>
#include <array>

namespace Yukiri::Detail
{
namespace
{
int ControlType(AccessibilityInfo::Role role)
{
	using Role = AccessibilityInfo::Role;
	switch (role)
	{
	case Role::Text: return UIA_TextControlTypeId;
	case Role::Image: return UIA_ImageControlTypeId;
	case Role::Button: return UIA_ButtonControlTypeId;
	case Role::CheckBox: case Role::Switch: return UIA_CheckBoxControlTypeId;
	case Role::RadioButton: return UIA_RadioButtonControlTypeId;
	case Role::Slider: return UIA_SliderControlTypeId;
	case Role::ProgressBar: return UIA_ProgressBarControlTypeId;
	case Role::Edit: return UIA_EditControlTypeId;
	case Role::NumberBox: return UIA_SpinnerControlTypeId;
	case Role::ComboBox: return UIA_ComboBoxControlTypeId;
	case Role::Tab: return UIA_TabControlTypeId;
	case Role::TabItem: return UIA_TabItemControlTypeId;
	case Role::List: return UIA_ListControlTypeId;
	case Role::ListItem: return UIA_ListItemControlTypeId;
	case Role::Tree: return UIA_TreeControlTypeId;
	case Role::TreeItem: return UIA_TreeItemControlTypeId;
	case Role::Table: return UIA_TableControlTypeId;
	case Role::DataItem: return UIA_DataItemControlTypeId;
	case Role::Header: return UIA_HeaderControlTypeId;
	case Role::HeaderItem: return UIA_HeaderItemControlTypeId;
	case Role::MenuBar: return UIA_MenuBarControlTypeId;
	case Role::Menu: return UIA_MenuControlTypeId;
	case Role::MenuItem: return UIA_MenuItemControlTypeId;
	case Role::ToolBar: return UIA_ToolBarControlTypeId;
	case Role::ToolTip: return UIA_ToolTipControlTypeId;
	case Role::StatusBar: return UIA_StatusBarControlTypeId;
	case Role::Separator: return UIA_SeparatorControlTypeId;
	case Role::Dialog: case Role::Window: return UIA_WindowControlTypeId;
	case Role::TitleBar: return UIA_TitleBarControlTypeId;
	case Role::ScrollView: return UIA_PaneControlTypeId;
	default: return UIA_GroupControlTypeId;
	}
}
}
HRESULT AutomationProvider::GetPropertyValue(PROPERTYID id, VARIANT* result)
{
	if (!result) return E_POINTER;
	VariantInit(result);
	return Read([&]() -> HRESULT
	{
		const auto state = m_Context->m_Tree->Describe(m_Node);
		auto boolean = [&](bool value) { result->vt = VT_BOOL; result->boolVal = value ? VARIANT_TRUE : VARIANT_FALSE; return S_OK; };
		auto integer = [&](int value) { result->vt = VT_I4; result->lVal = value; return S_OK; };
		auto number = [&](double value) { result->vt = VT_R8; result->dblVal = value; return S_OK; };
		auto string = [&](std::string_view value) { result->vt = VT_BSTR; return StringValue(value, &result->bstrVal); };
		auto supports = [&](AccessiblePattern pattern) { return boolean((state.m_Patterns & static_cast<std::uint64_t>(pattern)) != 0); };
		switch (id)
		{
		case UIA_ControlTypePropertyId: return integer(ControlType(state.m_Info.m_Role));
		case UIA_NamePropertyId: return string(state.m_Info.m_Name);
		case UIA_HelpTextPropertyId: return string(state.m_Info.m_Description);
		case UIA_AutomationIdPropertyId: return string(state.m_Info.m_AutomationId);
		case UIA_FrameworkIdPropertyId: return string("Yukiri");
		case UIA_ProviderDescriptionPropertyId: return string("Yukiri Windows UI Automation provider");
		case UIA_AccessKeyPropertyId: return string(state.m_AccessKey);
		case UIA_AcceleratorKeyPropertyId: return string(state.m_Accelerator);
		case UIA_ItemStatusPropertyId: return string(state.m_ItemStatus);
		case UIA_ProcessIdPropertyId: return integer(static_cast<int>(GetCurrentProcessId()));
		case UIA_NativeWindowHandlePropertyId: return integer(m_Node.m_Kind == AccessibleKind::Window ? static_cast<int>(reinterpret_cast<INT_PTR>(m_Context->m_Handle)) : 0);
		case UIA_IsEnabledPropertyId: return boolean(state.m_Enabled && (!m_Context->m_Handle || IsWindowEnabled(m_Context->m_Handle)));
		case UIA_IsOffscreenPropertyId: return boolean(!state.m_Visible);
		case UIA_IsKeyboardFocusablePropertyId: return boolean(state.m_Focusable);
		case UIA_HasKeyboardFocusPropertyId: return boolean(m_Node.m_Kind == AccessibleKind::Window ? m_Context->m_Handle && ::GetFocus() == m_Context->m_Handle && !m_Context->m_Tree->Focus() : state.m_Focused && (!m_Context->m_Handle || GetForegroundWindow() == m_Context->m_Handle));
		case UIA_IsPasswordPropertyId: return boolean(state.m_Password);
		case UIA_IsControlElementPropertyId: return boolean(state.m_Info.m_Role != AccessibilityInfo::Role::Group || !state.m_Info.m_Name.empty() || state.m_Info.m_IsContentElement.value_or(false));
		case UIA_IsContentElementPropertyId: return boolean(state.m_Info.m_IsContentElement.value_or(true));
		case UIA_IsDialogPropertyId: return boolean(state.m_Info.m_Role == AccessibilityInfo::Role::Dialog);
		case UIA_LiveSettingPropertyId: return integer(state.m_Info.m_Role == AccessibilityInfo::Role::StatusBar || state.m_Info.m_Role == AccessibilityInfo::Role::ToolTip ? LiveSetting::Polite : LiveSetting::Off);
		case UIA_LabeledByPropertyId:
			if (state.m_Info.m_Label)
			{
				const auto label = m_Context->m_Tree->ForWidget(*state.m_Info.m_Label);
				if (m_Context->m_Tree->IsAvailable(label)) { result->vt = VT_UNKNOWN; result->punkVal = m_Context->Get(label).Detach(); }
			}
			return S_OK;
		case UIA_BoundingRectanglePropertyId:
		case UIA_ClickablePointPropertyId:
		{
			UiaRect rect{}; const auto bounded = get_BoundingRectangle(&rect); if (FAILED(bounded)) return bounded;
			if (id == UIA_ClickablePointPropertyId && (rect.width <= 0 || rect.height <= 0)) return S_OK;
			result->vt = VT_ARRAY | VT_R8;
			if (id == UIA_ClickablePointPropertyId) return DoubleArray(std::array<double, 2>{rect.left + rect.width / 2, rect.top + rect.height / 2}, &result->parray);
			return DoubleArray(std::array<double, 4>{rect.left, rect.top, rect.width, rect.height}, &result->parray);
		}
		case UIA_IsInvokePatternAvailablePropertyId: return supports(AccessiblePattern::Invoke);
		case UIA_IsTogglePatternAvailablePropertyId: return supports(AccessiblePattern::Toggle);
		case UIA_IsSelectionPatternAvailablePropertyId: return supports(AccessiblePattern::Selection);
		case UIA_IsSelectionItemPatternAvailablePropertyId: return supports(AccessiblePattern::SelectionItem);
		case UIA_IsExpandCollapsePatternAvailablePropertyId: return supports(AccessiblePattern::ExpandCollapse);
		case UIA_IsValuePatternAvailablePropertyId: return supports(AccessiblePattern::Value);
		case UIA_IsRangeValuePatternAvailablePropertyId: return supports(AccessiblePattern::RangeValue);
		case UIA_IsScrollPatternAvailablePropertyId: return supports(AccessiblePattern::Scroll);
		case UIA_IsScrollItemPatternAvailablePropertyId: return supports(AccessiblePattern::ScrollItem);
		case UIA_IsGridPatternAvailablePropertyId: return supports(AccessiblePattern::Grid);
		case UIA_IsGridItemPatternAvailablePropertyId: return supports(AccessiblePattern::GridItem);
		case UIA_IsTablePatternAvailablePropertyId: return supports(AccessiblePattern::Table);
		case UIA_IsTableItemPatternAvailablePropertyId: return supports(AccessiblePattern::TableItem);
		case UIA_IsTextPatternAvailablePropertyId: case UIA_IsTextPattern2AvailablePropertyId: return supports(AccessiblePattern::Text);
		case UIA_IsWindowPatternAvailablePropertyId: return supports(AccessiblePattern::Window);
		case UIA_IsItemContainerPatternAvailablePropertyId: return supports(AccessiblePattern::ItemContainer);
		case UIA_IsVirtualizedItemPatternAvailablePropertyId: return supports(AccessiblePattern::VirtualizedItem);
		case UIA_SelectionItemIsSelectedPropertyId: return m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::SelectionItem) ? boolean(state.m_Selected) : S_OK;
		case UIA_ToggleToggleStatePropertyId: return m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Toggle) ? integer(state.m_Checked ? ToggleState_On : ToggleState_Off) : S_OK;
		case UIA_ExpandCollapseExpandCollapseStatePropertyId: return m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::ExpandCollapse) ? integer(state.m_Leaf ? ExpandCollapseState_LeafNode : state.m_Expanded ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed) : S_OK;
		case UIA_ValueValuePropertyId: return m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Value) && !state.m_Password ? string(m_Context->m_Tree->Value(m_Node)) : S_OK;
		case UIA_ValueIsReadOnlyPropertyId: case UIA_RangeValueIsReadOnlyPropertyId: return boolean(state.m_ReadOnly);
		case UIA_RangeValueValuePropertyId: case UIA_RangeValueMinimumPropertyId: case UIA_RangeValueMaximumPropertyId: case UIA_RangeValueSmallChangePropertyId: case UIA_RangeValueLargeChangePropertyId:
		{
			const auto range = m_Context->m_Tree->Range(m_Node); if (!range) return S_OK;
			return number(id == UIA_RangeValueValuePropertyId ? range->m_Value : id == UIA_RangeValueMinimumPropertyId ? range->m_Minimum : id == UIA_RangeValueMaximumPropertyId ? range->m_Maximum : id == UIA_RangeValueSmallChangePropertyId ? range->m_SmallChange : range->m_LargeChange);
		}
		default: return S_OK;
		}
	});
}
}
