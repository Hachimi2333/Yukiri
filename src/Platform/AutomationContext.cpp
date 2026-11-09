#include "AutomationContext.hpp"
#include "AutomationProvider.hpp"
#include "AutomationTextRange.hpp"
#include <algorithm>

namespace Yukiri::Detail
{
AutomationContext::AutomationContext(WidgetTree& tree, HWND handle) : m_Tree(std::make_shared<AccessibilityTree>(tree)), m_Handle(handle) {}
bool AutomationContext::IsPropertyObserved(PROPERTYID id)
{
	std::lock_guard lock(m_Mutex);
	const auto found = m_Subscriptions.find(id);
	return m_AllProperties || (found != m_Subscriptions.end() && found->second > 0);
}
ComPtr<IRawElementProviderSimple> AutomationContext::Get(const AccessibleNode& node)
{
	std::lock_guard lock(m_Mutex);
	if (const auto found = m_Providers.find(node); found != m_Providers.end() && !found->second->m_Disconnected)
	{
		found->second->AddRef();
		return ComPtr<IRawElementProviderSimple>(found->second);
	}
	auto* provider = new AutomationProvider(shared_from_this(), node);
	try { m_Providers.insert_or_assign(node, provider); }
	catch (...) { delete provider; throw; }
	return ComPtr<IRawElementProviderSimple>(provider);
}
std::optional<AccessibleNode> AutomationContext::Identity(IRawElementProviderSimple* provider)
{
	if (!provider) return {};
	ComPtr<IUnknown> identity;
	if (FAILED(provider->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(identity.Put())))) return {};
	std::lock_guard lock(m_Mutex);
	for (const auto& [node, candidate] : m_Providers) if (static_cast<IRawElementProviderSimple*>(candidate) == identity.Get()) return node;
	return {};
}
HRESULT AutomationContext::NodesArray(std::span<const AccessibleNode> nodes, SAFEARRAY** result)
{
	std::vector<ComPtr<IUnknown>> providers;
	providers.reserve(nodes.size());
	for (const auto& node : nodes) providers.push_back(ComPtr<IUnknown>(Get(node).Detach()));
	return UnknownArray(providers, result);
}
std::shared_ptr<TextRangeRegistry> AutomationContext::TextRegistry(const AccessibleNode& node)
{
	std::lock_guard lock(m_Mutex);
	auto& weak = m_TextRegistries[node];
	auto registry = weak.lock();
	if (!registry) { registry = std::make_shared<TextRangeRegistry>(); weak = registry; }
	return registry;
}
ComPtr<ITextRangeProvider> AutomationContext::TextRange(const AccessibleNode& node, std::size_t start, std::size_t end)
{
	return ComPtr<ITextRangeProvider>(new AutomationTextRange(shared_from_this(), node, start, end));
}
UiaRect AutomationContext::Screen(Rect rectangle) const
{
	if (rectangle.Empty()) return {};
	POINT origin{};
	const double scale = m_Handle ? static_cast<double>(GetDpiForWindow(m_Handle)) / 96 : 1;
	if (m_Handle) ClientToScreen(m_Handle, &origin);
	return {origin.x + rectangle.m_X * scale, origin.y + rectangle.m_Y * scale, rectangle.m_Width * scale, rectangle.m_Height * scale};
}
Point AutomationContext::Local(UiaPoint point) const
{
	POINT origin{};
	const double scale = m_Handle ? static_cast<double>(GetDpiForWindow(m_Handle)) / 96 : 1;
	if (m_Handle) ClientToScreen(m_Handle, &origin);
	return {static_cast<float>((point.x - origin.x) / scale), static_cast<float>((point.y - origin.y) / scale)};
}
void AutomationContext::Flush()
{
	if (!m_Alive || !m_Handle || m_Thread != std::this_thread::get_id()) return;
	std::vector<ComPtr<AutomationProvider>> providers;
	{
		std::lock_guard lock(m_Mutex);
		providers.reserve(m_Providers.size());
		for (const auto& [node, provider] : m_Providers) { (void)node; provider->AddRef(); providers.push_back(ComPtr<AutomationProvider>(provider)); }
		std::erase_if(m_TextRegistries, [](const auto& entry) { return entry.second.expired(); });
	}
	for (const auto& provider : providers)
	{
		const auto& node = provider->m_Node;
		const bool permanent = node.m_Lifetime.expired() || ((node.m_Kind == AccessibleKind::DataRow || node.m_Kind == AccessibleKind::DataCell || node.m_Kind == AccessibleKind::ComboItem || node.m_Kind == AccessibleKind::MenuHeader) && !m_Tree->IsAvailable(node));
		if (permanent && !provider->m_Disconnected) { provider->m_Disconnected = true; UiaDisconnectProvider(static_cast<IRawElementProviderSimple*>(provider.Get())); }
	}
	if (!UiaClientsAreListening()) return;
	for (const auto& provider : providers) if (!provider->m_Disconnected) provider->Flush();
	const auto structure = m_Tree->StructureVersion();
	if (structure != m_LastStructure)
	{
		auto root = Get(m_Tree->Root());
		UiaRaiseStructureChangedEvent(root.Get(), StructureChangeType_ChildrenInvalidated, nullptr, 0);
		m_LastStructure = structure;
	}
	const auto focus = GetForegroundWindow() == m_Handle && IsWindowEnabled(m_Handle) ? m_Tree->Focus() : std::nullopt;
	if (focus && focus != m_LastFocus)
	{
		auto provider = Get(*focus);
		UiaRaiseAutomationEvent(provider.Get(), UIA_AutomationFocusChangedEventId);
	}
	m_LastFocus = focus;
}
void AutomationContext::Shutdown() noexcept
{
	if (!m_Alive.exchange(false)) return;
	decltype(m_Providers) providers;
	{
		std::lock_guard lock(m_Mutex);
		providers.swap(m_Providers);
		for (const auto& [node, provider] : providers) { (void)node; provider->AddRef(); provider->m_Disconnected = true; }
	}
	for (const auto& [node, provider] : providers)
	{
		(void)node;
		if (m_Handle) UiaDisconnectProvider(static_cast<IRawElementProviderSimple*>(provider));
		provider->Release();
	}
	m_Handle = nullptr;
}
}
