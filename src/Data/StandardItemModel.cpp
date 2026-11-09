#include <Yukiri/Data/StandardItemModel.hpp>
#include "Text/Unicode.hpp"
#include <limits>

namespace Yukiri
{
StandardItemModel::StandardItemModel(std::vector<std::string> headers) : m_Headers(std::move(headers))
{
	if (m_Headers.empty()) m_Headers.emplace_back("Name");
	m_Nodes.emplace(0, Node{});
}
std::size_t StandardItemModel::GetRowCount(ItemId parent) const { const auto found = m_Nodes.find(parent); return found == m_Nodes.end() ? 0 : found->second.m_Children.size(); }
std::size_t StandardItemModel::GetColumnCount() const { return m_Headers.size(); }
ItemId StandardItemModel::GetChild(ItemId parent, std::size_t row) const
{
	const auto found = m_Nodes.find(parent);
	return found != m_Nodes.end() && row < found->second.m_Children.size() ? found->second.m_Children[row] : 0;
}
ItemId StandardItemModel::GetParent(ItemId item) const { const auto found = m_Nodes.find(item); return found == m_Nodes.end() ? 0 : found->second.m_Parent; }
bool StandardItemModel::Contains(ItemId item) const { return item != 0 && m_Nodes.contains(item); }
ItemModel::Cell StandardItemModel::GetCell(ItemId item, std::size_t column) const
{
	const auto found = m_Nodes.find(item);
	return found != m_Nodes.end() && column < found->second.m_Cells.size() ? found->second.m_Cells[column] : Cell{{}, {}, false, false, false, {}};
}
std::string StandardItemModel::GetHeader(std::size_t column) const { return column < m_Headers.size() ? m_Headers[column] : std::string{}; }
Result<ItemId> StandardItemModel::AppendRow(std::vector<Cell> cells, ItemId parent) { return InsertRow(GetRowCount(parent), std::move(cells), parent); }
Result<ItemId> StandardItemModel::InsertRow(std::size_t row, std::vector<Cell> cells, ItemId parent)
{
	AssertThread();
	if (!m_Nodes.contains(parent) || row > GetRowCount(parent) || m_NextId == std::numeric_limits<ItemId>::max()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid parent, row, or exhausted item IDs."});
	const auto keepAlive = weak_from_this().lock();
	const auto sorting = m_Sort;
	if (cells.size() > m_Headers.size()) return std::unexpected(Error{ErrorCode::InvalidArgument, "The row has more cells than the model has columns."});
	cells.resize(m_Headers.size());
	for (auto& cell : cells) cell.m_Text = Detail::NormalizeText(cell.m_Text, true);
	const auto id = m_NextId++;
	m_Nodes.emplace(id, Node{parent, std::move(cells), {}});
	auto& children = m_Nodes.at(parent).m_Children;
	children.insert(children.begin() + static_cast<std::ptrdiff_t>(row), id);
	Notify({ChangeKind::Inserted, parent, row, 1, {id}});
	if (sorting && m_Sort == sorting) { const auto sorted = Sort(sorting->first, sorting->second); (void)sorted; }
	return id;
}
Result<std::vector<ItemId>> StandardItemModel::AppendRows(std::vector<std::vector<Cell>> rows, ItemId parent)
{
	AssertThread();
	if (!m_Nodes.contains(parent) || rows.size() >= std::numeric_limits<ItemId>::max() - m_NextId) return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid parent or exhausted item IDs."});
	const auto keepAlive = weak_from_this().lock();
	const auto sorting = m_Sort;
	for (const auto& cells : rows) if (cells.size() > m_Headers.size()) return std::unexpected(Error{ErrorCode::InvalidArgument, "A row has more cells than the model has columns."});
	std::vector<ItemId> ids;
	const auto first = GetRowCount(parent);
	for (auto& cells : rows)
	{
		cells.resize(m_Headers.size());
		for (auto& cell : cells) cell.m_Text = Detail::NormalizeText(cell.m_Text, true);
		const auto id = m_NextId++;
		m_Nodes.emplace(id, Node{parent, std::move(cells), {}});
		m_Nodes.at(parent).m_Children.push_back(id);
		ids.push_back(id);
	}
	if (!ids.empty())
	{
		Notify({ChangeKind::Inserted, parent, first, ids.size(), ids});
		if (sorting && m_Sort == sorting) { const auto sorted = Sort(sorting->first, sorting->second); (void)sorted; }
	}
	return ids;
}
Result<void> StandardItemModel::Remove(ItemId item)
{
	AssertThread();
	if (!Contains(item)) return std::unexpected(Error{ErrorCode::InvalidArgument, "The item does not exist."});
	const auto parent = GetParent(item);
	auto& children = m_Nodes.at(parent).m_Children;
	const auto found = std::ranges::find(children, item);
	const auto row = static_cast<std::size_t>(found - children.begin());
	children.erase(found);
	std::vector<ItemId> removed{item};
	for (std::size_t index = 0; index < removed.size(); ++index)
	{
		const auto& descendants = m_Nodes.at(removed[index]).m_Children;
		removed.insert(removed.end(), descendants.begin(), descendants.end());
	}
	for (const auto id : removed) m_Nodes.erase(id);
	Notify({ChangeKind::Removed, parent, row, 1, std::move(removed)});
	return {};
}
Result<void> StandardItemModel::Move(ItemId item, ItemId parent, std::size_t row)
{
	AssertThread();
	if (!Contains(item) || !m_Nodes.contains(parent) || row > GetRowCount(parent)) return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid move target."});
	for (auto ancestor = parent; ancestor; ancestor = GetParent(ancestor)) if (ancestor == item) return std::unexpected(Error{ErrorCode::InvalidArgument, "An item cannot contain itself."});
	auto& previous = m_Nodes.at(GetParent(item)).m_Children;
	const auto old = std::ranges::find(previous, item);
	if (GetParent(item) == parent && static_cast<std::size_t>(old - previous.begin()) < row) --row;
	previous.erase(old);
	auto& target = m_Nodes.at(parent).m_Children;
	target.insert(target.begin() + static_cast<std::ptrdiff_t>(row), item);
	m_Nodes.at(item).m_Parent = parent;
	m_Sort.reset();
	Notify({ChangeKind::Reordered, parent, row, 1, {item}});
	return {};
}
void StandardItemModel::Clear()
{
	AssertThread();
	m_Nodes.clear(); m_Nodes.emplace(0, Node{}); m_Sort.reset();
	Notify({ChangeKind::Reset, 0, 0, 0, {}});
}
void StandardItemModel::SetHeaders(std::vector<std::string> headers)
{
	AssertThread();
	if (headers.empty()) headers.emplace_back("Name");
	m_Headers = std::move(headers);
	for (auto& [id, node] : m_Nodes) if (id) node.m_Cells.resize(m_Headers.size());
	m_Sort.reset();
	Notify({ChangeKind::Reset, 0, 0, 0, {}});
}
Result<void> StandardItemModel::SetCell(ItemId item, std::size_t column, Cell cell)
{
	AssertThread();
	if (!Contains(item) || column >= m_Headers.size()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid cell."});
	const auto keepAlive = weak_from_this().lock();
	const auto sorting = m_Sort;
	cell.m_Text = Detail::NormalizeText(cell.m_Text, true);
	if (m_Nodes.at(item).m_Cells[column] == cell) return {};
	m_Nodes.at(item).m_Cells[column] = std::move(cell);
	Notify({ChangeKind::Updated, GetParent(item), 0, 1, {item}});
	if (sorting && sorting->first == column && m_Sort == sorting && Contains(item)) { const auto sorted = Sort(sorting->first, sorting->second); (void)sorted; }
	return {};
}
Result<void> StandardItemModel::SetText(ItemId item, std::size_t column, std::string text)
{
	const auto cell = GetCell(item, column);
	if (!cell.m_Enabled || !cell.m_Editable) return std::unexpected(Error{ErrorCode::InvalidArgument, "The cell is not editable."});
	auto updated = cell; updated.m_Text = std::move(text);
	return SetCell(item, column, std::move(updated));
}
Result<void> StandardItemModel::SetChecked(ItemId item, std::size_t column, bool checked)
{
	const auto cell = GetCell(item, column);
	if (!cell.m_Enabled || !cell.m_Checked) return std::unexpected(Error{ErrorCode::InvalidArgument, "The cell is not checkable."});
	auto updated = cell; updated.m_Checked = checked;
	return SetCell(item, column, std::move(updated));
}
Result<void> StandardItemModel::Sort(std::size_t column, bool ascending)
{
	AssertThread();
	if (column >= m_Headers.size()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid sort column."});
	for (auto& [id, node] : m_Nodes)
	{
		(void)id;
		std::stable_sort(node.m_Children.begin(), node.m_Children.end(), [&](auto left, auto right)
		{
			const auto& a = m_Nodes.at(left).m_Cells[column].m_Text;
			const auto& b = m_Nodes.at(right).m_Cells[column].m_Text;
			return ascending ? a < b : b < a;
		});
	}
	m_Sort = std::pair(column, ascending);
	Notify({ChangeKind::Reordered, 0, 0, 0, {}});
	return {};
}
std::optional<std::pair<std::size_t, bool>> StandardItemModel::GetSort() const { return m_Sort; }
}
