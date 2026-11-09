#include <Yukiri/Data/ItemModel.hpp>
#include <cassert>

namespace Yukiri
{
std::string ItemModel::GetHeader(std::size_t column) const { return "Column " + std::to_string(column + 1); }
Result<void> ItemModel::SetText(ItemId, std::size_t, std::string) { return std::unexpected(Error{ErrorCode::InvalidArgument, "The model does not support text editing."}); }
Result<void> ItemModel::SetChecked(ItemId, std::size_t, bool) { return std::unexpected(Error{ErrorCode::InvalidArgument, "The model does not support checkbox editing."}); }
Result<void> ItemModel::Sort(std::size_t, bool) { return std::unexpected(Error{ErrorCode::InvalidArgument, "The model does not support sorting."}); }
std::optional<std::pair<std::size_t, bool>> ItemModel::GetSort() const { return {}; }
void ItemModel::AssertThread() const { assert(m_Thread == std::this_thread::get_id()); }
void ItemModel::Notify(Change change)
{
	AssertThread();
	const auto keepAlive = weak_from_this().lock();
	m_Changed.Emit(change);
}
}
