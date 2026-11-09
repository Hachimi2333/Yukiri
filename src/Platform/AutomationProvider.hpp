#pragma once

#include "AutomationContext.hpp"
#include "AutomationTextProvider.hpp"

namespace Yukiri::Detail
{
class AutomationProvider final : public IRawElementProviderSimple, public IRawElementProviderFragment, public IRawElementProviderFragmentRoot, public IRawElementProviderAdviseEvents,
	public IInvokeProvider, public IToggleProvider, public ISelectionProvider, public ISelectionItemProvider, public IExpandCollapseProvider,
	public IValueProvider, public IRangeValueProvider, public IScrollProvider, public IScrollItemProvider, public IGridProvider, public IGridItemProvider,
	public ITableProvider, public ITableItemProvider, public IWindowProvider, public IVirtualizedItemProvider, public IItemContainerProvider
{
public:
	AutomationProvider(std::shared_ptr<AutomationContext> context, AccessibleNode node);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override;
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* result) override;
	HRESULT STDMETHODCALLTYPE AdviseEventAdded(EVENTID id, SAFEARRAY* properties) override;
	HRESULT STDMETHODCALLTYPE AdviseEventRemoved(EVENTID id, SAFEARRAY* properties) override;
	HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id, IUnknown** result) override;
	HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* result) override;
	HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** result) override;
	HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** result) override;
	HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* result) override;
	HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE SetFocus() override;
	HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** result) override;
	HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** result) override;
	HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** result) override;
	HRESULT STDMETHODCALLTYPE Invoke() override;
	HRESULT STDMETHODCALLTYPE Toggle() override;
	HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState* result) override;
	HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE Select() override;
	HRESULT STDMETHODCALLTYPE AddToSelection() override;
	HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
	HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple** result) override;
	HRESULT STDMETHODCALLTYPE Expand() override;
	HRESULT STDMETHODCALLTYPE Collapse() override;
	HRESULT STDMETHODCALLTYPE get_ExpandCollapseState(ExpandCollapseState* result) override;
	HRESULT STDMETHODCALLTYPE SetValue(LPCWSTR value) override;
	HRESULT STDMETHODCALLTYPE SetValue(double value) override;
	HRESULT STDMETHODCALLTYPE get_Value(BSTR* result) override;
	HRESULT STDMETHODCALLTYPE get_Value(double* result) override;
	HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE get_Maximum(double* result) override;
	HRESULT STDMETHODCALLTYPE get_Minimum(double* result) override;
	HRESULT STDMETHODCALLTYPE get_LargeChange(double* result) override;
	HRESULT STDMETHODCALLTYPE get_SmallChange(double* result) override;
	HRESULT STDMETHODCALLTYPE Scroll(ScrollAmount horizontal, ScrollAmount vertical) override;
	HRESULT STDMETHODCALLTYPE SetScrollPercent(double horizontal, double vertical) override;
	HRESULT STDMETHODCALLTYPE get_HorizontalScrollPercent(double* result) override;
	HRESULT STDMETHODCALLTYPE get_VerticalScrollPercent(double* result) override;
	HRESULT STDMETHODCALLTYPE get_HorizontalViewSize(double* result) override;
	HRESULT STDMETHODCALLTYPE get_VerticalViewSize(double* result) override;
	HRESULT STDMETHODCALLTYPE get_HorizontallyScrollable(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE get_VerticallyScrollable(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE ScrollIntoView() override;
	HRESULT STDMETHODCALLTYPE GetItem(int row, int column, IRawElementProviderSimple** result) override;
	HRESULT STDMETHODCALLTYPE get_RowCount(int* result) override;
	HRESULT STDMETHODCALLTYPE get_ColumnCount(int* result) override;
	HRESULT STDMETHODCALLTYPE get_Row(int* result) override;
	HRESULT STDMETHODCALLTYPE get_Column(int* result) override;
	HRESULT STDMETHODCALLTYPE get_RowSpan(int* result) override;
	HRESULT STDMETHODCALLTYPE get_ColumnSpan(int* result) override;
	HRESULT STDMETHODCALLTYPE get_ContainingGrid(IRawElementProviderSimple** result) override;
	HRESULT STDMETHODCALLTYPE GetRowHeaders(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE GetColumnHeaders(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE get_RowOrColumnMajor(RowOrColumnMajor* result) override;
	HRESULT STDMETHODCALLTYPE GetRowHeaderItems(SAFEARRAY** result) override;
	HRESULT STDMETHODCALLTYPE GetColumnHeaderItems(SAFEARRAY** result) override;
	HRESULT GetTextSelection(SAFEARRAY** result);
	HRESULT GetVisibleRanges(SAFEARRAY** result);
	HRESULT RangeFromChild(IRawElementProviderSimple* child, ITextRangeProvider** result);
	HRESULT RangeFromPoint(UiaPoint point, ITextRangeProvider** result);
	HRESULT get_DocumentRange(ITextRangeProvider** result);
	HRESULT get_SupportedTextSelection(SupportedTextSelection* result);
	HRESULT RangeFromAnnotation(IRawElementProviderSimple* annotation, ITextRangeProvider** result);
	HRESULT GetCaretRange(BOOL* active, ITextRangeProvider** result);
	HRESULT STDMETHODCALLTYPE SetVisualState(WindowVisualState state) override;
	HRESULT STDMETHODCALLTYPE Close() override;
	HRESULT STDMETHODCALLTYPE WaitForInputIdle(int milliseconds, BOOL* result) override;
	HRESULT STDMETHODCALLTYPE get_CanMaximize(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE get_CanMinimize(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE get_IsModal(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE get_WindowVisualState(WindowVisualState* result) override;
	HRESULT STDMETHODCALLTYPE get_WindowInteractionState(WindowInteractionState* result) override;
	HRESULT STDMETHODCALLTYPE get_IsTopmost(BOOL* result) override;
	HRESULT STDMETHODCALLTYPE Realize() override;
	HRESULT STDMETHODCALLTYPE FindItemByProperty(IRawElementProviderSimple* after, PROPERTYID property, VARIANT value, IRawElementProviderSimple** result) override;
	void Flush();
	AccessibleNode m_Node;
	bool m_Disconnected = false;
private:
	friend class AutomationContext;
	~AutomationProvider() = default;
	template<typename F> HRESULT Read(F&& action) { return m_Context->Read(m_Node, std::forward<F>(action)); }
	template<typename F> HRESULT Action(AccessiblePattern pattern, F&& action)
	{
		return Read([&]() -> HRESULT
		{
			const auto state = m_Context->m_Tree->Describe(m_Node);
			if (!(state.m_Patterns & static_cast<std::uint64_t>(pattern))) return UIA_E_NOTSUPPORTED;
			if (!state.m_Enabled || (m_Context->m_Handle && !IsWindowEnabled(m_Context->m_Handle))) return UIA_E_ELEMENTNOTENABLED;
			return action() ? S_OK : UIA_E_INVALIDOPERATION;
		});
	}
	HRESULT RangeProperty(double* result, int property);
	HRESULT ScrollProperty(double* result, bool horizontal, bool viewSize);
	std::shared_ptr<AutomationContext> m_Context;
	AutomationTextProvider m_TextProvider;
	std::atomic<ULONG> m_References{1};
	std::optional<AccessibleState> m_LastState;
	std::uint64_t m_LastTextVersion = 0;
	std::pair<std::size_t, std::size_t> m_LastTextSelection;
	UiaRect m_LastScreen{};
	std::optional<AccessibleRange> m_LastRange;
	std::optional<AccessibleScroll> m_LastScroll;
	int m_LastWindowState = -1;
	std::uint64_t m_LastSelectionVersion = 0;
	std::string m_LastTextValue;
	bool m_RecordingTextValue = false;
	HRESULT Advise(EVENTID id, SAFEARRAY* properties, bool added);
	std::unordered_map<PROPERTYID, unsigned> m_Subscriptions;
	unsigned m_AllProperties = 0;
};
}
