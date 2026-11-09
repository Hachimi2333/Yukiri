#pragma once

#include "AutomationContext.hpp"

namespace Yukiri::Detail
{
class AutomationTextRange final : public ITextRangeProvider
{
public:
	AutomationTextRange(std::shared_ptr<AutomationContext> context, AccessibleNode node, std::size_t start, std::size_t end);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override;
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE Clone(ITextRangeProvider** result) override;
	HRESULT STDMETHODCALLTYPE Compare(ITextRangeProvider* range, BOOL* result) override;
	HRESULT STDMETHODCALLTYPE CompareEndpoints(TextPatternRangeEndpoint endpoint, ITextRangeProvider* target, TextPatternRangeEndpoint targetEndpoint, int* result) override;
	HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit(TextUnit unit) override;
	HRESULT STDMETHODCALLTYPE FindAttribute(TEXTATTRIBUTEID id, VARIANT value, BOOL backward, ITextRangeProvider** result) override;
	HRESULT STDMETHODCALLTYPE FindText(BSTR text, BOOL backward, BOOL ignoreCase, ITextRangeProvider** result) override;
	HRESULT STDMETHODCALLTYPE GetAttributeValue(TEXTATTRIBUTEID id, VARIANT* result) override;
	HRESULT STDMETHODCALLTYPE GetBoundingRectangles(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE GetEnclosingElement(IRawElementProviderSimple** result) override;
	HRESULT STDMETHODCALLTYPE GetText(int maximum, BSTR* result) override;
	HRESULT STDMETHODCALLTYPE Move(TextUnit unit, int count, int* result) override;
	HRESULT STDMETHODCALLTYPE MoveEndpointByUnit(TextPatternRangeEndpoint endpoint, TextUnit unit, int count, int* result) override;
	HRESULT STDMETHODCALLTYPE MoveEndpointByRange(TextPatternRangeEndpoint endpoint, ITextRangeProvider* target, TextPatternRangeEndpoint targetEndpoint) override;
	HRESULT STDMETHODCALLTYPE Select() override;
	HRESULT STDMETHODCALLTYPE AddToSelection() override;
	HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
	HRESULT STDMETHODCALLTYPE ScrollIntoView(BOOL top) override;
	HRESULT STDMETHODCALLTYPE GetChildren(SAFEARRAY** result) override;
private:
	~AutomationTextRange();
	std::shared_ptr<TextRangeAnchor> Peer(ITextRangeProvider* range);
	template<typename F> HRESULT Read(F&& action)
	{
		return m_Context->Read(m_Node, [&]() -> HRESULT
		{
			if (m_Context->m_Tree->Describe(m_Node).m_Password) return E_ACCESSDENIED;
			if (!m_Context->m_Tree->TextControl(m_Node)) return UIA_E_NOTSUPPORTED;
			const auto version = m_Context->m_Tree->TextVersion(m_Node);
			if (m_Registry->m_Version != version)
			{
				auto text = m_Context->m_Tree->Value(m_Node);
				std::lock_guard lock(m_Context->m_Mutex);
				m_Registry->Sync(std::move(text), version);
			}
			return action();
		});
	}
	std::shared_ptr<AutomationContext> m_Context;
	AccessibleNode m_Node;
	std::shared_ptr<TextRangeRegistry> m_Registry;
	std::shared_ptr<TextRangeAnchor> m_Anchor;
	std::atomic<ULONG> m_References{1};
};
}
