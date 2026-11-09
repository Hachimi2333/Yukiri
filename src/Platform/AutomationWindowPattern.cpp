#include "AutomationProvider.hpp"

namespace Yukiri::Detail
{
HRESULT AutomationProvider::SetVisualState(WindowVisualState state)
{
	if (state != WindowVisualState_Normal && state != WindowVisualState_Minimized && state != WindowVisualState_Maximized) return E_INVALIDARG;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Window)) return UIA_E_NOTSUPPORTED;
		if (m_Node.m_Kind != AccessibleKind::Window) return UIA_E_INVALIDOPERATION;
		if (!m_Context->m_Tree->Describe(m_Node).m_Enabled || (m_Context->m_Handle && !IsWindowEnabled(m_Context->m_Handle))) return UIA_E_ELEMENTNOTENABLED;
		if (state == WindowVisualState_Maximized && m_Context->m_Handle && IsZoomed(m_Context->m_Handle)) return S_OK;
		return m_Context->m_Tree->WindowAction(m_Node, state == WindowVisualState_Normal ? 3 : state == WindowVisualState_Minimized ? 0 : 1) ? S_OK : UIA_E_INVALIDOPERATION;
	});
}
HRESULT AutomationProvider::Close() { return Action(AccessiblePattern::Window, [&] { return m_Context->m_Tree->WindowAction(m_Node, 2); }); }
HRESULT AutomationProvider::WaitForInputIdle(int milliseconds, BOOL* result)
{
	if (!result) return E_POINTER;
	if (milliseconds < 0) return E_INVALIDARG;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Window)) return UIA_E_NOTSUPPORTED; *result = TRUE; return S_OK; });
}
HRESULT AutomationProvider::get_CanMaximize(BOOL* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Window)) return UIA_E_NOTSUPPORTED; *result = m_Node.m_Kind == AccessibleKind::Window; return S_OK; });
}
HRESULT AutomationProvider::get_CanMinimize(BOOL* result) { return get_CanMaximize(result); }
HRESULT AutomationProvider::get_IsModal(BOOL* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Window)) return UIA_E_NOTSUPPORTED; *result = m_Context->m_Tree->Describe(m_Node).m_Modal; return S_OK; });
}
HRESULT AutomationProvider::get_WindowVisualState(WindowVisualState* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Window)) return UIA_E_NOTSUPPORTED;
		*result = m_Node.m_Kind == AccessibleKind::Window && m_Context->m_Handle ? IsIconic(m_Context->m_Handle) ? WindowVisualState_Minimized : IsZoomed(m_Context->m_Handle) ? WindowVisualState_Maximized : WindowVisualState_Normal : WindowVisualState_Normal;
		return S_OK;
	});
}
HRESULT AutomationProvider::get_WindowInteractionState(WindowInteractionState* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Window)) return UIA_E_NOTSUPPORTED;
		const auto state = m_Context->m_Tree->Describe(m_Node);
		*result = state.m_ItemStatus == "Blocked" || (m_Context->m_Handle && !IsWindowEnabled(m_Context->m_Handle)) ? WindowInteractionState_BlockedByModalWindow : WindowInteractionState_ReadyForUserInteraction;
		return S_OK;
	});
}
HRESULT AutomationProvider::get_IsTopmost(BOOL* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Window)) return UIA_E_NOTSUPPORTED; *result = m_Context->m_Handle && (GetWindowLongPtrW(m_Context->m_Handle, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0; return S_OK; });
}
}
