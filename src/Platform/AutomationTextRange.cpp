#include "AutomationTextRange.hpp"
#include "Text/Unicode.hpp"
#include <limits>

namespace Yukiri::Detail
{
void TextRangeRegistry::Sync(std::string text, std::uint64_t version)
{
	if (m_Version == version) return;
	std::size_t prefix = 0, suffix = 0;
	while (prefix < m_Text.size() && prefix < text.size() && m_Text[prefix] == text[prefix]) ++prefix;
	while (suffix < m_Text.size() - prefix && suffix < text.size() - prefix && m_Text[m_Text.size() - suffix - 1] == text[text.size() - suffix - 1]) ++suffix;
	m_Boundaries = GraphemeBoundaries(text);
	const auto& boundaries = m_Boundaries;
	auto floor = [&](std::size_t value) { auto after = std::ranges::upper_bound(boundaries, std::min(value, text.size())); return after == boundaries.begin() ? std::size_t(0) : *std::prev(after); };
	prefix = floor(prefix);
	const auto oldEnd = m_Text.size() - suffix, newEnd = text.size() - suffix;
	for (auto entry = m_Anchors.begin(); entry != m_Anchors.end();)
	{
		if (auto anchor = entry->second.lock())
		{
			const bool collapsed = anchor->m_Start == anchor->m_End;
			auto map = [&](std::size_t value, bool right)
			{
				if (value < prefix || (value == prefix && !right)) return floor(value);
				if (value >= oldEnd) return floor(newEnd + value - oldEnd);
				return floor(right ? newEnd : prefix);
			};
			anchor->m_Start = map(anchor->m_Start, collapsed);
			anchor->m_End = map(anchor->m_End, true);
			if (anchor->m_End < anchor->m_Start) anchor->m_End = anchor->m_Start;
			++entry;
		}
		else entry = m_Anchors.erase(entry);
	}
	m_Text = std::move(text); m_Version = version;
}
AutomationTextRange::AutomationTextRange(std::shared_ptr<AutomationContext> context, AccessibleNode node, std::size_t start, std::size_t end)
	: m_Context(std::move(context)), m_Node(std::move(node)), m_Registry(m_Context->TextRegistry(m_Node)), m_Anchor(std::make_shared<TextRangeAnchor>())
{
	const auto version = m_Context->m_Tree->TextVersion(m_Node);
	auto text = m_Registry->m_Version == version ? std::string{} : m_Context->m_Tree->Value(m_Node);
	std::lock_guard lock(m_Context->m_Mutex);
	if (m_Registry->m_Version != version) m_Registry->Sync(std::move(text), version);
	const auto& boundaries = m_Registry->m_Boundaries;
	auto floor = [&](std::size_t value) { auto after = std::ranges::upper_bound(boundaries, std::min(value, m_Registry->m_Text.size())); return after == boundaries.begin() ? std::size_t(0) : *std::prev(after); };
	m_Anchor->m_Start = floor(std::min(start, end)); m_Anchor->m_End = floor(std::max(start, end));
	m_Registry->m_Anchors.emplace(static_cast<ITextRangeProvider*>(this), m_Anchor);
}
AutomationTextRange::~AutomationTextRange()
{
	std::lock_guard lock(m_Context->m_Mutex);
	m_Registry->m_Anchors.erase(static_cast<ITextRangeProvider*>(this));
}
HRESULT AutomationTextRange::QueryInterface(REFIID iid, void** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	if (iid != __uuidof(IUnknown) && iid != __uuidof(ITextRangeProvider)) return E_NOINTERFACE;
	*result = static_cast<ITextRangeProvider*>(this); AddRef(); return S_OK;
}
ULONG AutomationTextRange::AddRef() { return ++m_References; }
ULONG AutomationTextRange::Release() { const auto remaining = --m_References; if (!remaining) delete this; return remaining; }
std::shared_ptr<TextRangeAnchor> AutomationTextRange::Peer(ITextRangeProvider* range)
{
	if (!range) return {};
	ComPtr<IUnknown> identity;
	if (FAILED(range->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(identity.Put())))) return {};
	std::lock_guard lock(m_Context->m_Mutex);
	const auto found = m_Registry->m_Anchors.find(identity.Get());
	return found == m_Registry->m_Anchors.end() ? std::shared_ptr<TextRangeAnchor>{} : found->second.lock();
}
HRESULT AutomationTextRange::Clone(ITextRangeProvider** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&] { *result = m_Context->TextRange(m_Node, m_Anchor->m_Start, m_Anchor->m_End).Detach(); return S_OK; });
}
HRESULT AutomationTextRange::Compare(ITextRangeProvider* range, BOOL* result)
{
	if (!result) return E_POINTER;
	*result = FALSE;
	return Read([&]() -> HRESULT { const auto peer = Peer(range); if (!peer) return E_INVALIDARG; *result = peer->m_Start == m_Anchor->m_Start && peer->m_End == m_Anchor->m_End; return S_OK; });
}
HRESULT AutomationTextRange::CompareEndpoints(TextPatternRangeEndpoint endpoint, ITextRangeProvider* target, TextPatternRangeEndpoint targetEndpoint, int* result)
{
	if (!result) return E_POINTER;
	*result = 0;
	if ((endpoint != TextPatternRangeEndpoint_Start && endpoint != TextPatternRangeEndpoint_End) || (targetEndpoint != TextPatternRangeEndpoint_Start && targetEndpoint != TextPatternRangeEndpoint_End)) return E_INVALIDARG;
	return Read([&]() -> HRESULT
	{
		const auto peer = Peer(target); if (!peer) return E_INVALIDARG;
		const auto left = endpoint == TextPatternRangeEndpoint_Start ? m_Anchor->m_Start : m_Anchor->m_End;
		const auto right = targetEndpoint == TextPatternRangeEndpoint_Start ? peer->m_Start : peer->m_End;
		*result = left == right ? 0 : left < right ? -1 : 1;
		return S_OK;
	});
}
HRESULT AutomationTextRange::MoveEndpointByRange(TextPatternRangeEndpoint endpoint, ITextRangeProvider* target, TextPatternRangeEndpoint targetEndpoint)
{
	if ((endpoint != TextPatternRangeEndpoint_Start && endpoint != TextPatternRangeEndpoint_End) || (targetEndpoint != TextPatternRangeEndpoint_Start && targetEndpoint != TextPatternRangeEndpoint_End)) return E_INVALIDARG;
	return Read([&]() -> HRESULT
	{
		const auto peer = Peer(target); if (!peer) return E_INVALIDARG;
		const auto position = targetEndpoint == TextPatternRangeEndpoint_Start ? peer->m_Start : peer->m_End;
		if (endpoint == TextPatternRangeEndpoint_Start) { m_Anchor->m_Start = position; m_Anchor->m_End = std::max(m_Anchor->m_End, position); }
		else { m_Anchor->m_End = position; m_Anchor->m_Start = std::min(m_Anchor->m_Start, position); }
		return S_OK;
	});
}
}
