#pragma once

#include <Yukiri/Data/ItemModel.hpp>
#include <unordered_map>

namespace Yukiri
{
class StandardItemModel : public ItemModel
{
public:
	explicit StandardItemModel(std::vector<std::string> headers = {"Name"});
	[[nodiscard]] Result<ItemId> AppendRow(std::vector<Cell> cells, ItemId parent = 0);
	[[nodiscard]] Result<std::vector<ItemId>> AppendRows(std::vector<std::vector<Cell>> rows, ItemId parent = 0);
	[[nodiscard]] Result<ItemId> InsertRow(std::size_t row, std::vector<Cell> cells, ItemId parent = 0);
	[[nodiscard]] Result<void> Remove(ItemId item);
	[[nodiscard]] Result<void> Move(ItemId item, ItemId parent, std::size_t row);
	void Clear();
	void SetHeaders(std::vector<std::string> headers);
	[[nodiscard]] Result<void> SetCell(ItemId item, std::size_t column, Cell cell);
	std::size_t GetRowCount(ItemId parent = 0) const override;
	std::size_t GetColumnCount() const override;
	ItemId GetChild(ItemId parent, std::size_t row) const override;
	ItemId GetParent(ItemId item) const override;
	bool Contains(ItemId item) const override;
	Cell GetCell(ItemId item, std::size_t column = 0) const override;
	std::string GetHeader(std::size_t column) const override;
	Result<void> SetText(ItemId item, std::size_t column, std::string text) override;
	Result<void> SetChecked(ItemId item, std::size_t column, bool checked) override;
	Result<void> Sort(std::size_t column, bool ascending = true) override;
	std::optional<std::pair<std::size_t, bool>> GetSort() const override;
private:
	struct Node { ItemId m_Parent = 0; std::vector<Cell> m_Cells; std::vector<ItemId> m_Children; };
	std::unordered_map<ItemId, Node> m_Nodes;
	std::vector<std::string> m_Headers;
	std::optional<std::pair<std::size_t, bool>> m_Sort;
	ItemId m_NextId = 1;
};
}
