#pragma once

#include "AutomationSupport.hpp"

namespace Yukiri::Detail
{
class AutomationProvider;
class AutomationTextProvider final : public ITextProvider2
{
public:
	explicit AutomationTextProvider(AutomationProvider& owner) : m_Owner(owner) {}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override;
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE GetVisibleRanges(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE RangeFromChild(IRawElementProviderSimple* child, ITextRangeProvider** result) override;
	HRESULT STDMETHODCALLTYPE RangeFromPoint(UiaPoint point, ITextRangeProvider** result) override;
	HRESULT STDMETHODCALLTYPE get_DocumentRange(ITextRangeProvider** result) override;
	HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(SupportedTextSelection* result) override;
	HRESULT STDMETHODCALLTYPE RangeFromAnnotation(IRawElementProviderSimple* annotation, ITextRangeProvider** result) override;
	HRESULT STDMETHODCALLTYPE GetCaretRange(BOOL* active, ITextRangeProvider** result) override;
private:
	AutomationProvider& m_Owner;
};
}
