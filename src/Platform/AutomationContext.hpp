#pragma once

#include "AutomationSupport.hpp"
#include "UI/AccessibilityTree.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <stdexcept>
#include <new>

namespace Yukiri::Detail
{
class AutomationProvider;
class AutomationTextRange;
struct TextRangeAnchor { std::size_t m_Start; std::size_t m_End; };
struct TextRangeRegistry
{
	std::string m_Text;
	std::uint64_t m_Version = ~std::uint64_t(0);
	std::vector<std::size_t> m_Boundaries{0};
	std::unordered_map<IUnknown*, std::weak_ptr<TextRangeAnchor>> m_Anchors;
	void Sync(std::string text, std::uint64_t version);
};
struct AccessibleHash
{
	std::size_t operator()(const AccessibleNode& node) const noexcept
	{
		auto hash = static_cast<std::size_t>(node.m_WidgetId);
		for (auto part : {static_cast<std::uint64_t>(node.m_Kind), node.m_Item, static_cast<std::uint64_t>(node.m_Index), node.m_Generation}) hash ^= static_cast<std::size_t>(part) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		return hash;
	}
};
class AutomationContext : public std::enable_shared_from_this<AutomationContext>
{
public:
	explicit AutomationContext(WidgetTree& tree, HWND handle = nullptr);
	ComPtr<IRawElementProviderSimple> Get(const AccessibleNode& node);
	std::optional<AccessibleNode> Identity(IRawElementProviderSimple* provider);
	HRESULT NodesArray(std::span<const AccessibleNode> nodes, SAFEARRAY** result);
	std::shared_ptr<TextRangeRegistry> TextRegistry(const AccessibleNode& node);
	ComPtr<ITextRangeProvider> TextRange(const AccessibleNode& node, std::size_t start, std::size_t end);
	UiaRect Screen(Rect rectangle) const;
	Point Local(UiaPoint point) const;
	void Flush();
	void Shutdown() noexcept;
	std::shared_ptr<AccessibilityTree> m_Tree;
	HWND m_Handle = nullptr;
	std::thread::id m_Thread = std::this_thread::get_id();
	std::atomic<bool> m_Alive{true};
	std::mutex m_Mutex;
	std::unordered_map<AccessibleNode, AutomationProvider*, AccessibleHash> m_Providers;
	std::unordered_map<AccessibleNode, std::weak_ptr<TextRangeRegistry>, AccessibleHash> m_TextRegistries;
	std::optional<AccessibleNode> m_LastFocus;
	std::uint64_t m_LastStructure = 0;
	std::unordered_map<PROPERTYID, unsigned> m_Subscriptions;
	unsigned m_AllProperties = 0;
	bool IsPropertyObserved(PROPERTYID id);
	template<typename F> HRESULT Read(const AccessibleNode& node, F&& action)
	{
		if (!m_Alive) return UIA_E_ELEMENTNOTAVAILABLE;
		if (m_Thread != std::this_thread::get_id()) return RPC_E_WRONG_THREAD;
		HRESULT result = UIA_E_ELEMENTNOTAVAILABLE;
		try
		{
			m_Tree->Dispatch([&] { m_Tree->EnsureLayout(); if (m_Alive && m_Tree->IsAvailable(node)) result = action(); });
		}
		catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
		catch (const std::invalid_argument&) { return E_INVALIDARG; }
		catch (...) { return E_FAIL; }
		return result;
	}
};
}
