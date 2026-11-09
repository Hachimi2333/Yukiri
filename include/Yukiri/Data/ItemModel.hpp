#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Core/Signal.hpp>
#include <Yukiri/Render/Image.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <utility>

namespace Yukiri
{
using ItemId = std::uint64_t;
class ItemModel : public std::enable_shared_from_this<ItemModel>
{
public:
	struct Cell
	{
		std::string m_Text;
		Image m_Icon;
		bool m_Enabled = true;
		bool m_Selectable = true;
		bool m_Editable = false;
		std::optional<bool> m_Checked;
		bool operator==(const Cell&) const = default;
	};
	enum class ChangeKind { Inserted, Removed, Updated, Reordered, Reset };
	struct Change
	{
		ChangeKind m_Kind = ChangeKind::Reset;
		ItemId m_Parent = 0;
		std::size_t m_First = 0;
		std::size_t m_Count = 0;
		std::vector<ItemId> m_Items;
	};
	virtual ~ItemModel() = default;
	[[nodiscard]] virtual std::size_t GetRowCount(ItemId parent = 0) const = 0;
	[[nodiscard]] virtual std::size_t GetColumnCount() const = 0;
	[[nodiscard]] virtual ItemId GetChild(ItemId parent, std::size_t row) const = 0;
	[[nodiscard]] virtual ItemId GetParent(ItemId item) const = 0;
	[[nodiscard]] virtual bool Contains(ItemId item) const = 0;
	[[nodiscard]] virtual Cell GetCell(ItemId item, std::size_t column = 0) const = 0;
	[[nodiscard]] virtual std::string GetHeader(std::size_t column) const;
	[[nodiscard]] virtual Result<void> SetText(ItemId item, std::size_t column, std::string text);
	[[nodiscard]] virtual Result<void> SetChecked(ItemId item, std::size_t column, bool checked);
	[[nodiscard]] virtual Result<void> Sort(std::size_t column, bool ascending = true);
	[[nodiscard]] virtual std::optional<std::pair<std::size_t, bool>> GetSort() const;
	Signal<const Change&> m_Changed;
protected:
	ItemModel() = default;
	void Notify(Change change);
	void AssertThread() const;
private:
	std::thread::id m_Thread = std::this_thread::get_id();
};
}
