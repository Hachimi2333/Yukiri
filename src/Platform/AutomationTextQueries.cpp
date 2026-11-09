#include "AutomationTextRange.hpp"
#include <Yukiri/Controls/TextBox.hpp>
#include "Text/TextEditor.hpp"

namespace Yukiri::Detail
{
HRESULT AutomationTextRange::GetText(int maximum, BSTR* result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	if (maximum < -1) return E_INVALIDARG;
	return Read([&]() -> HRESULT
	{
		auto wide = ToWide(std::string_view(m_Registry->m_Text).substr(m_Anchor->m_Start, m_Anchor->m_End - m_Anchor->m_Start));
		if (maximum >= 0 && wide.size() > static_cast<std::size_t>(maximum))
		{
			wide.resize(static_cast<std::size_t>(maximum));
			if (!wide.empty() && wide.back() >= 0xD800 && wide.back() <= 0xDBFF) wide.pop_back();
		}
		*result = SysAllocStringLen(wide.data(), static_cast<UINT>(wide.size()));
		return *result ? S_OK : E_OUTOFMEMORY;
	});
}
HRESULT AutomationTextRange::FindText(BSTR text, BOOL backward, BOOL ignoreCase, ITextRangeProvider** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	if (!text) return E_INVALIDARG;
	return Read([&]
	{
		TextEditor editor(false);
		editor.SetText(std::string_view(m_Registry->m_Text).substr(m_Anchor->m_Start, m_Anchor->m_End - m_Anchor->m_Start));
		const auto found = editor.Find(ToUtf8(std::wstring_view(text, SysStringLen(text))), backward ? editor.m_Text.size() : 0, !ignoreCase, backward != FALSE);
		if (found) *result = m_Context->TextRange(m_Node, m_Anchor->m_Start + found->first, m_Anchor->m_Start + found->second).Detach();
		return S_OK;
	});
}
HRESULT AutomationTextRange::GetAttributeValue(TEXTATTRIBUTEID id, VARIANT* result)
{
	if (!result) return E_POINTER;
	VariantInit(result);
	return Read([&]() -> HRESULT
	{
		auto* text = m_Context->m_Tree->TextControl(m_Node);
		switch (id)
		{
		case UIA_IsReadOnlyAttributeId: result->vt = VT_BOOL; result->boolVal = text->IsReadOnly() ? VARIANT_TRUE : VARIANT_FALSE; return S_OK;
		case UIA_FontNameAttributeId: result->vt = VT_BSTR; return StringValue(text->GetFont().m_Family.empty() ? "Default" : text->GetFont().m_Family, &result->bstrVal);
		case UIA_FontSizeAttributeId: result->vt = VT_R8; result->dblVal = text->GetFontSize() * 0.75; return S_OK;
		case UIA_FontWeightAttributeId: result->vt = VT_I4; result->lVal = text->GetFont().m_Weight; return S_OK;
		case UIA_IsItalicAttributeId: result->vt = VT_BOOL; result->boolVal = text->GetFont().m_Italic ? VARIANT_TRUE : VARIANT_FALSE; return S_OK;
		default: result->vt = VT_UNKNOWN; return UiaGetReservedNotSupportedValue(&result->punkVal);
		}
	});
}
HRESULT AutomationTextRange::FindAttribute(TEXTATTRIBUTEID id, VARIANT value, BOOL, ITextRangeProvider** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		VARIANT current{};
		const auto read = GetAttributeValue(id, &current);
		if (FAILED(read)) return read;
		const bool matches = current.vt != VT_UNKNOWN && VarCmp(&current, &value, LOCALE_INVARIANT, 0) == VARCMP_EQ;
		VariantClear(&current);
		if (matches) *result = m_Context->TextRange(m_Node, m_Anchor->m_Start, m_Anchor->m_End).Detach();
		return S_OK;
	});
}
HRESULT AutomationTextRange::GetBoundingRectangles(SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]
	{
		std::vector<double> values;
		for (const auto bounds : m_Context->m_Tree->TextRectangles(m_Node, m_Anchor->m_Start, m_Anchor->m_End))
		{
			const auto rect = m_Context->Screen(bounds);
			values.insert(values.end(), {rect.left, rect.top, rect.width, rect.height});
		}
		return DoubleArray(values, result);
	});
}
HRESULT AutomationTextRange::GetEnclosingElement(IRawElementProviderSimple** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&] { *result = m_Context->Get(m_Node).Detach(); return S_OK; });
}
HRESULT AutomationTextRange::Select()
{
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->Describe(m_Node).m_Enabled) return UIA_E_ELEMENTNOTENABLED;
		m_Context->m_Tree->TextControl(m_Node)->SetSelection(m_Anchor->m_Start, m_Anchor->m_End);
		return S_OK;
	});
}
HRESULT AutomationTextRange::AddToSelection() { return Read([]() -> HRESULT { return UIA_E_INVALIDOPERATION; }); }
HRESULT AutomationTextRange::RemoveFromSelection() { return Read([]() -> HRESULT { return UIA_E_INVALIDOPERATION; }); }
HRESULT AutomationTextRange::ScrollIntoView(BOOL top) { return Read([&]() -> HRESULT { return m_Context->m_Tree->ScrollText(m_Node, m_Anchor->m_Start, m_Anchor->m_End, top != FALSE) ? S_OK : UIA_E_INVALIDOPERATION; }); }
HRESULT AutomationTextRange::GetChildren(SAFEARRAY** result) { if (!result) return E_POINTER; *result = nullptr; return Read([&] { return UnknownArray({}, result); }); }
}
