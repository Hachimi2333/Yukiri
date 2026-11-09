#include "AutomationProvider.hpp"
#include <Yukiri/Controls/TextBox.hpp>
#include <array>
#include <cmath>

namespace Yukiri::Detail
{
HRESULT AutomationTextProvider::QueryInterface(REFIID iid, void** result) { return m_Owner.QueryInterface(iid, result); }
ULONG AutomationTextProvider::AddRef() { return m_Owner.AddRef(); }
ULONG AutomationTextProvider::Release() { return m_Owner.Release(); }
HRESULT AutomationTextProvider::GetSelection(SAFEARRAY** result) { return m_Owner.GetTextSelection(result); }
HRESULT AutomationTextProvider::GetVisibleRanges(SAFEARRAY** result) { return m_Owner.GetVisibleRanges(result); }
HRESULT AutomationTextProvider::RangeFromChild(IRawElementProviderSimple* child, ITextRangeProvider** result) { return m_Owner.RangeFromChild(child, result); }
HRESULT AutomationTextProvider::RangeFromPoint(UiaPoint point, ITextRangeProvider** result) { return m_Owner.RangeFromPoint(point, result); }
HRESULT AutomationTextProvider::get_DocumentRange(ITextRangeProvider** result) { return m_Owner.get_DocumentRange(result); }
HRESULT AutomationTextProvider::get_SupportedTextSelection(SupportedTextSelection* result) { return m_Owner.get_SupportedTextSelection(result); }
HRESULT AutomationTextProvider::RangeFromAnnotation(IRawElementProviderSimple* annotation, ITextRangeProvider** result) { return m_Owner.RangeFromAnnotation(annotation, result); }
HRESULT AutomationTextProvider::GetCaretRange(BOOL* active, ITextRangeProvider** result) { return m_Owner.GetCaretRange(active, result); }
HRESULT AutomationProvider::GetTextSelection(SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		auto* text = m_Context->m_Tree->TextControl(m_Node); if (!text) return UIA_E_NOTSUPPORTED;
		const auto selection = text->GetSelection();
		std::array<ComPtr<IUnknown>, 1> ranges{ComPtr<IUnknown>(m_Context->TextRange(m_Node, std::min(selection.first, selection.second), std::max(selection.first, selection.second)).Detach())};
		return UnknownArray(ranges, result);
	});
}
HRESULT AutomationProvider::GetVisibleRanges(SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->TextControl(m_Node)) return UIA_E_NOTSUPPORTED;
		if (!m_Context->m_Tree->Describe(m_Node).m_Visible) return UnknownArray({}, result);
		const auto visible = m_Context->m_Tree->VisibleText(m_Node);
		std::array<ComPtr<IUnknown>, 1> ranges{ComPtr<IUnknown>(m_Context->TextRange(m_Node, visible.first, visible.second).Detach())};
		return UnknownArray(ranges, result);
	});
}
HRESULT AutomationProvider::RangeFromChild(IRawElementProviderSimple*, ITextRangeProvider** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT { return m_Context->m_Tree->TextControl(m_Node) ? E_INVALIDARG : UIA_E_NOTSUPPORTED; });
}
HRESULT AutomationProvider::RangeFromPoint(UiaPoint point, ITextRangeProvider** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	if (!std::isfinite(point.x) || !std::isfinite(point.y)) return E_INVALIDARG;
	return Read([&]() -> HRESULT
	{
		auto* text = m_Context->m_Tree->TextControl(m_Node); if (!text) return UIA_E_NOTSUPPORTED;
		const auto offset = text->HitTestOffset(m_Context->Local(point));
		*result = m_Context->TextRange(m_Node, offset, offset).Detach(); return S_OK;
	});
}
HRESULT AutomationProvider::get_DocumentRange(ITextRangeProvider** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		auto* text = m_Context->m_Tree->TextControl(m_Node); if (!text) return UIA_E_NOTSUPPORTED;
		*result = m_Context->TextRange(m_Node, 0, text->GetText().size()).Detach(); return S_OK;
	});
}
HRESULT AutomationProvider::get_SupportedTextSelection(SupportedTextSelection* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->TextControl(m_Node)) return UIA_E_NOTSUPPORTED; *result = SupportedTextSelection_Single; return S_OK; });
}
HRESULT AutomationProvider::RangeFromAnnotation(IRawElementProviderSimple*, ITextRangeProvider** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([]() -> HRESULT { return UIA_E_NOTSUPPORTED; });
}
HRESULT AutomationProvider::GetCaretRange(BOOL* active, ITextRangeProvider** result)
{
	if (!active || !result) return E_POINTER;
	*active = FALSE; *result = nullptr;
	return Read([&]() -> HRESULT
	{
		auto* text = m_Context->m_Tree->TextControl(m_Node); if (!text) return UIA_E_NOTSUPPORTED;
		*active = text->HasFocus() && (!m_Context->m_Handle || GetForegroundWindow() == m_Context->m_Handle);
		const auto offset = text->GetSelection().second;
		*result = m_Context->TextRange(m_Node, offset, offset).Detach(); return S_OK;
	});
}
}
