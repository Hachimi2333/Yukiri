#pragma once

#include "ImageData.hpp"
#include <algorithm>
#include <map>
#include <tuple>

namespace Yukiri::Detail
{
class ImageCache
{
public:
	using Key = std::tuple<std::uint64_t, std::uint32_t, std::uint32_t>;
	struct Entry
	{
		std::weak_ptr<const ImageData> m_Source;
		std::uint32_t m_Texture = 0;
		std::uint64_t m_LastUsed = 0;
		std::size_t m_Bytes = 0;
	};
	explicit ImageCache(std::size_t budget = 64 * 1024 * 1024) : m_Budget(budget) {}
	Entry* Find(const Key& key)
	{
		auto found = m_Entries.find(key);
		if (found == m_Entries.end()) return nullptr;
		found->second.m_LastUsed = ++m_Tick;
		return &found->second;
	}
	void Insert(Key key, std::weak_ptr<const ImageData> source, std::uint32_t texture, std::size_t bytes)
	{
		m_Entries.emplace(std::move(key), Entry{std::move(source), texture, ++m_Tick, bytes});
	}
	template<typename InUse, typename Retire>
	void Collect(InUse inUse, Retire retire)
	{
		std::size_t bytes = 0;
		std::vector<decltype(m_Entries)::iterator> entries;
		for (auto entry = m_Entries.begin(); entry != m_Entries.end(); ++entry)
		{
			bytes += entry->second.m_Bytes;
			entries.push_back(entry);
		}
		std::ranges::sort(entries, {}, [](const auto& entry) { return entry->second.m_LastUsed; });
		for (auto entry : entries)
		{
			if ((!entry->second.m_Source.expired() && bytes <= m_Budget) || inUse(entry->second.m_Texture)) continue;
			bytes -= entry->second.m_Bytes;
			retire(entry->second.m_Texture);
			m_Entries.erase(entry);
		}
	}
	[[nodiscard]] std::size_t GetCount() const noexcept { return m_Entries.size(); }
private:
	std::map<Key, Entry> m_Entries;
	std::size_t m_Budget;
	std::uint64_t m_Tick = 0;
};
}
