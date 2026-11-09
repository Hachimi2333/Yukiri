#include "AutomationProvider.hpp"
#include <array>
#include <cmath>

namespace Yukiri::Detail
{
AutomationProvider::AutomationProvider(std::shared_ptr<AutomationContext> context, AccessibleNode node) : m_Node(std::move(node)), m_Context(std::move(context)), m_TextProvider(*this) {}
HRESULT AutomationProvider::QueryInterface(REFIID iid, void** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	if (iid == __uuidof(IUnknown) || iid == __uuidof(IRawElementProviderSimple)) *result = static_cast<IRawElementProviderSimple*>(this);
	else if (iid == __uuidof(IRawElementProviderFragment)) *result = static_cast<IRawElementProviderFragment*>(this);
	else if (iid == __uuidof(IRawElementProviderAdviseEvents)) *result = static_cast<IRawElementProviderAdviseEvents*>(this);
	else if (iid == __uuidof(IRawElementProviderFragmentRoot) && m_Node.m_Kind == AccessibleKind::Window) *result = static_cast<IRawElementProviderFragmentRoot*>(this);
	else if (iid == __uuidof(IInvokeProvider)) *result = static_cast<IInvokeProvider*>(this);
	else if (iid == __uuidof(IToggleProvider)) *result = static_cast<IToggleProvider*>(this);
	else if (iid == __uuidof(ISelectionProvider)) *result = static_cast<ISelectionProvider*>(this);
	else if (iid == __uuidof(ISelectionItemProvider)) *result = static_cast<ISelectionItemProvider*>(this);
	else if (iid == __uuidof(IExpandCollapseProvider)) *result = static_cast<IExpandCollapseProvider*>(this);
	else if (iid == __uuidof(IValueProvider)) *result = static_cast<IValueProvider*>(this);
	else if (iid == __uuidof(IRangeValueProvider)) *result = static_cast<IRangeValueProvider*>(this);
	else if (iid == __uuidof(IScrollProvider)) *result = static_cast<IScrollProvider*>(this);
	else if (iid == __uuidof(IScrollItemProvider)) *result = static_cast<IScrollItemProvider*>(this);
	else if (iid == __uuidof(IGridProvider)) *result = static_cast<IGridProvider*>(this);
	else if (iid == __uuidof(IGridItemProvider)) *result = static_cast<IGridItemProvider*>(this);
	else if (iid == __uuidof(ITableProvider)) *result = static_cast<ITableProvider*>(this);
	else if (iid == __uuidof(ITableItemProvider)) *result = static_cast<ITableItemProvider*>(this);
	else if (iid == __uuidof(ITextProvider)) *result = static_cast<ITextProvider*>(&m_TextProvider);
	else if (iid == __uuidof(ITextProvider2)) *result = static_cast<ITextProvider2*>(&m_TextProvider);
	else if (iid == __uuidof(IWindowProvider)) *result = static_cast<IWindowProvider*>(this);
	else if (iid == __uuidof(IVirtualizedItemProvider)) *result = static_cast<IVirtualizedItemProvider*>(this);
	else if (iid == __uuidof(IItemContainerProvider)) *result = static_cast<IItemContainerProvider*>(this);
	if (!*result) return E_NOINTERFACE;
	AddRef(); return S_OK;
}
ULONG AutomationProvider::AddRef() { return ++m_References; }
ULONG AutomationProvider::Release()
{
	ULONG remaining;
	{
		std::lock_guard lock(m_Context->m_Mutex);
		remaining = --m_References;
		if (!remaining)
		{
			if (const auto found = m_Context->m_Providers.find(m_Node); found != m_Context->m_Providers.end() && found->second == this) m_Context->m_Providers.erase(found);
			for (const auto& [id, count] : m_Subscriptions) { auto& observed = m_Context->m_Subscriptions[id]; observed -= std::min(observed, count); }
			m_Context->m_AllProperties -= std::min(m_Context->m_AllProperties, m_AllProperties);
		}
	}
	if (!remaining) delete this;
	return remaining;
}
HRESULT AutomationProvider::get_ProviderOptions(ProviderOptions* result)
{
	if (!result) return E_POINTER;
	return Read([&] { *result = static_cast<ProviderOptions>(ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading |
		(m_Node.m_Kind == AccessibleKind::Window && m_Context->m_Tree->HasCustomTitleBar() ? ProviderOptions_RefuseNonClientSupport : 0)); return S_OK; });
}
HRESULT AutomationProvider::GetPatternProvider(PATTERNID id, IUnknown** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		AccessiblePattern pattern;
		IUnknown* provider = nullptr;
		switch (id)
		{
		case UIA_InvokePatternId: pattern = AccessiblePattern::Invoke; provider = static_cast<IInvokeProvider*>(this); break;
		case UIA_TogglePatternId: pattern = AccessiblePattern::Toggle; provider = static_cast<IToggleProvider*>(this); break;
		case UIA_SelectionPatternId: pattern = AccessiblePattern::Selection; provider = static_cast<ISelectionProvider*>(this); break;
		case UIA_SelectionItemPatternId: pattern = AccessiblePattern::SelectionItem; provider = static_cast<ISelectionItemProvider*>(this); break;
		case UIA_ExpandCollapsePatternId: pattern = AccessiblePattern::ExpandCollapse; provider = static_cast<IExpandCollapseProvider*>(this); break;
		case UIA_ValuePatternId: pattern = AccessiblePattern::Value; provider = static_cast<IValueProvider*>(this); break;
		case UIA_RangeValuePatternId: pattern = AccessiblePattern::RangeValue; provider = static_cast<IRangeValueProvider*>(this); break;
		case UIA_ScrollPatternId: pattern = AccessiblePattern::Scroll; provider = static_cast<IScrollProvider*>(this); break;
		case UIA_ScrollItemPatternId: pattern = AccessiblePattern::ScrollItem; provider = static_cast<IScrollItemProvider*>(this); break;
		case UIA_GridPatternId: pattern = AccessiblePattern::Grid; provider = static_cast<IGridProvider*>(this); break;
		case UIA_GridItemPatternId: pattern = AccessiblePattern::GridItem; provider = static_cast<IGridItemProvider*>(this); break;
		case UIA_TablePatternId: pattern = AccessiblePattern::Table; provider = static_cast<ITableProvider*>(this); break;
		case UIA_TableItemPatternId: pattern = AccessiblePattern::TableItem; provider = static_cast<ITableItemProvider*>(this); break;
		case UIA_TextPatternId: pattern = AccessiblePattern::Text; provider = static_cast<ITextProvider*>(&m_TextProvider); break;
		case UIA_TextPattern2Id: pattern = AccessiblePattern::Text; provider = static_cast<ITextProvider2*>(&m_TextProvider); break;
		case UIA_WindowPatternId: pattern = AccessiblePattern::Window; provider = static_cast<IWindowProvider*>(this); break;
		case UIA_VirtualizedItemPatternId: pattern = AccessiblePattern::VirtualizedItem; provider = static_cast<IVirtualizedItemProvider*>(this); break;
		case UIA_ItemContainerPatternId: pattern = AccessiblePattern::ItemContainer; provider = static_cast<IItemContainerProvider*>(this); break;
		default: return S_OK;
		}
		if (m_Context->m_Tree->HasPattern(m_Node, pattern)) { provider->AddRef(); *result = provider; }
		return S_OK;
	});
}
HRESULT AutomationProvider::get_HostRawElementProvider(IRawElementProviderSimple** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&] { return m_Node.m_Kind == AccessibleKind::Window && m_Context->m_Handle ? UiaHostProviderFromHwnd(m_Context->m_Handle, result) : S_OK; });
}
HRESULT AutomationProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		std::optional<AccessibleNode> target;
		auto& tree = *m_Context->m_Tree;
		switch (direction)
		{
		case NavigateDirection_Parent: target = tree.Parent(m_Node); break;
		case NavigateDirection_NextSibling: target = tree.Sibling(m_Node, true); break;
		case NavigateDirection_PreviousSibling: target = tree.Sibling(m_Node, false); break;
		case NavigateDirection_FirstChild: target = tree.Child(m_Node, 0); break;
		case NavigateDirection_LastChild: if (const auto count = tree.ChildCount(m_Node)) target = tree.Child(m_Node, count - 1); break;
		default: return E_INVALIDARG;
		}
		return target ? m_Context->Get(*target)->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(result)) : S_OK;
	});
}
HRESULT AutomationProvider::GetRuntimeId(SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (m_Node.m_Kind == AccessibleKind::Window) return S_OK;
		const auto index = static_cast<std::uint64_t>(m_Node.m_Index);
		const std::array<int, 10> parts{UiaAppendRuntimeId, static_cast<int>(m_Node.m_Kind), static_cast<int>(m_Node.m_WidgetId), static_cast<int>(m_Node.m_WidgetId >> 32),
			static_cast<int>(m_Node.m_Item), static_cast<int>(m_Node.m_Item >> 32), static_cast<int>(index), static_cast<int>(index >> 32), static_cast<int>(m_Node.m_Generation), static_cast<int>(m_Node.m_Generation >> 32)};
		*result = SafeArrayCreateVector(VT_I4, 0, static_cast<ULONG>(parts.size()));
		if (!*result) return E_OUTOFMEMORY;
		for (LONG part = 0; part < static_cast<LONG>(parts.size()); ++part) { auto value = parts[static_cast<std::size_t>(part)]; SafeArrayPutElement(*result, &part, &value); }
		return S_OK;
	});
}
HRESULT AutomationProvider::get_BoundingRectangle(UiaRect* result)
{
	if (!result) return E_POINTER;
	*result = {};
	return Read([&]
	{
		*result = m_Context->Screen(m_Context->m_Tree->Describe(m_Node).m_Bounds);
		if (m_Node.m_Kind == AccessibleKind::Window && m_Context->m_Handle && IsWindowVisible(m_Context->m_Handle) && !IsIconic(m_Context->m_Handle))
		{ RECT rect{}; GetWindowRect(m_Context->m_Handle, &rect); *result = {static_cast<double>(rect.left), static_cast<double>(rect.top), static_cast<double>(rect.right - rect.left), static_cast<double>(rect.bottom - rect.top)}; }
		return S_OK;
	});
}
HRESULT AutomationProvider::GetEmbeddedFragmentRoots(SAFEARRAY** result) { if (!result) return E_POINTER; *result = nullptr; return Read([] { return S_OK; }); }
HRESULT AutomationProvider::SetFocus()
{
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->Describe(m_Node).m_Enabled || (m_Context->m_Handle && !IsWindowEnabled(m_Context->m_Handle))) return UIA_E_ELEMENTNOTENABLED;
		if (m_Context->m_Handle && (!IsWindowVisible(m_Context->m_Handle) || IsIconic(m_Context->m_Handle))) return UIA_E_INVALIDOPERATION;
		return m_Context->m_Tree->SetFocus(m_Node) ? S_OK : UIA_E_INVALIDOPERATION;
	});
}
HRESULT AutomationProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&] { return m_Context->Get(m_Context->m_Tree->Root())->QueryInterface(__uuidof(IRawElementProviderFragmentRoot), reinterpret_cast<void**>(result)); });
}
HRESULT AutomationProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	if (!std::isfinite(x) || !std::isfinite(y)) return E_INVALIDARG;
	return Read([&]() -> HRESULT
	{
		if (m_Node.m_Kind != AccessibleKind::Window) return UIA_E_NOTSUPPORTED;
		const auto node = m_Context->m_Tree->AtPoint(m_Context->Local({x, y}));
		return node ? m_Context->Get(*node)->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(result)) : S_OK;
	});
}
HRESULT AutomationProvider::GetFocus(IRawElementProviderFragment** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (m_Node.m_Kind != AccessibleKind::Window) return UIA_E_NOTSUPPORTED;
		if (m_Context->m_Handle && GetForegroundWindow() != m_Context->m_Handle) return S_OK;
		const auto node = m_Context->m_Tree->Focus();
		if (!node && m_Context->m_Handle && ::GetFocus() == m_Context->m_Handle) return m_Context->Get(m_Context->m_Tree->Root())->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(result));
		return node ? m_Context->Get(*node)->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(result)) : S_OK;
	});
}
}
