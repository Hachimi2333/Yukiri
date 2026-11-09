#include <Yukiri/Data/SelectionModel.hpp>
#include <cassert>

namespace Yukiri
{
SelectionModel::SelectionModel(std::shared_ptr<ItemModel> model) { SetModel(std::move(model)); }
void SelectionModel::AssertThread() const { assert(m_Thread == std::this_thread::get_id()); }
void SelectionModel::SetModel(std::shared_ptr<ItemModel> model)
{
	AssertThread();
	m_Connection.Disconnect();
	m_Model = std::move(model);
	++m_Version;
	if (m_Model) m_Connection = m_Model->m_Changed.Connect([this](const auto& change)
	{
		if (change.m_Kind == ItemModel::ChangeKind::Reset || change.m_Kind == ItemModel::ChangeKind::Removed) Prune(change.m_Kind == ItemModel::ChangeKind::Reset);
	});
	Prune(true);
}
const std::shared_ptr<ItemModel>& SelectionModel::GetModel() const noexcept { return m_Model; }
SelectionModel::Mode SelectionModel::GetMode() const noexcept { return m_Mode; }
void SelectionModel::SetMode(Mode mode)
{
	AssertThread();
	if (m_Mode == mode) return;
	const auto keepAlive = weak_from_this().lock();
	m_Mode = mode;
	++m_Version;
	auto selected = m_Selected;
	if (mode == Mode::Single && IsSelected(m_Current)) selected = {m_Current};
	SetSelected(std::move(selected), m_Current);
	m_Changed.Emit();
}
const std::vector<ItemId>& SelectionModel::GetSelected() const noexcept { return m_Selected; }
ItemId SelectionModel::GetCurrent() const noexcept { return m_Current; }
std::uint64_t SelectionModel::GetVersion() const noexcept { return m_Version; }
bool SelectionModel::IsSelected(ItemId item) const { return m_Index.contains(item); }
void SelectionModel::Clear() { SetSelected({}, m_Current); }
void SelectionModel::SetSelected(std::vector<ItemId> items, ItemId current)
{
	AssertThread();
	const auto keepAlive = weak_from_this().lock();
	std::vector<ItemId> selected;
	std::unordered_set<ItemId> index;
	if (m_Model) for (const auto item : items)
	{
		if (!m_Model->Contains(item)) continue;
		const auto cell = m_Model->GetCell(item);
		if (!cell.m_Enabled || !cell.m_Selectable || !index.insert(item).second) continue;
		selected.push_back(item);
		if (m_Mode == Mode::Single) break;
	}
	if (!current && !selected.empty()) current = selected.front();
	if (current && (!m_Model || !m_Model->Contains(current))) current = 0;
	const bool changed = m_Selected != selected;
	const bool currentChanged = m_Current != current;
	m_Selected = std::move(selected); m_Index = std::move(index); m_Current = current;
	if (changed || currentChanged) ++m_Version;
	if (currentChanged) m_CurrentChanged.Emit(current);
	if (changed) m_Changed.Emit();
}
void SelectionModel::Select(ItemId item, bool add, bool toggle)
{
	auto selected = (add || toggle) && m_Mode == Mode::Multiple ? m_Selected : std::vector<ItemId>{};
	if (toggle && IsSelected(item)) std::erase(selected, item); else selected.push_back(item);
	SetSelected(std::move(selected), item);
}
void SelectionModel::SetCurrent(ItemId current)
{
	AssertThread();
	if (current && (!m_Model || !m_Model->Contains(current))) return;
	if (m_Current == current) return;
	const auto keepAlive = weak_from_this().lock();
	m_Current = current;
	++m_Version;
	m_CurrentChanged.Emit(current);
}
void SelectionModel::Prune(bool reset)
{
	if (reset) { SetSelected({}, 0); return; }
	SetSelected(m_Selected, m_Current);
}
}
