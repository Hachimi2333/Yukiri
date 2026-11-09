#include "ItemViewState.hpp"
#include "UI/AccessibilityTree.hpp"
#include <cmath>

namespace Yukiri
{
ListView::ListView() : ListView(ViewKind::List) {}
ListView::ListView(ViewKind kind) : m_Impl(std::make_unique<Impl>())
{
	m_Impl->m_Kind = kind;
	SetFocusable(true);
	m_Impl->m_Selection = std::make_shared<SelectionModel>();
	BindSelection();
}
ListView::~ListView() { m_Impl->m_Destroying = true; }
const std::shared_ptr<ItemModel>& ListView::GetModel() const noexcept { return m_Impl->m_Model; }
const std::shared_ptr<SelectionModel>& ListView::GetSelectionModel() const noexcept { return m_Impl->m_Selection; }
void ListView::SetModel(std::shared_ptr<ItemModel> model)
{
	Dispatch([&]
	{
		CancelEdit();
		m_Impl->m_ModelConnection.Disconnect();
		m_Impl->m_Model = std::move(model);
		++m_Impl->m_ModelGeneration;
		const auto selectionMode = m_Impl->m_Selection->GetMode();
		m_Impl->m_Selection = std::make_shared<SelectionModel>(m_Impl->m_Model);
		m_Impl->m_Selection->SetMode(selectionMode);
		m_Impl->m_Root = m_Impl->m_Anchor = 0;
		m_Impl->m_Expanded.clear();
		m_Impl->m_Offset = m_Impl->m_Target = {};
		m_Impl->m_ScrollX.Snap(0); m_Impl->m_ScrollY.Snap(0);
		BindSelection();
		if (m_Impl->m_Model)
		{
			const auto lifetime = ObserveLifetime();
			m_Impl->m_ModelConnection = m_Impl->m_Model->m_Changed.Connect([this, lifetime](const auto& change)
			{
				if (!lifetime.expired()) Dispatch([&] { ModelChanged(change); });
			});
		}
		RebuildRows();
	});
}
void ListView::BindSelection()
{
	m_Impl->m_SelectionConnections.clear();
	const auto lifetime = ObserveLifetime();
	m_Impl->m_SelectionConnections.push_back(m_Impl->m_Selection->m_Changed.Connect([this, lifetime] { if (!lifetime.expired()) InvalidatePaint(); }));
	m_Impl->m_SelectionConnections.push_back(m_Impl->m_Selection->m_CurrentChanged.Connect([this, lifetime](auto) { if (!lifetime.expired()) InvalidatePaint(); }));
}
Result<void> ListView::SetSelectionModel(std::shared_ptr<SelectionModel> selection)
{
	if (!selection || selection->GetModel() != m_Impl->m_Model) return std::unexpected(Error{ErrorCode::InvalidArgument, "A shared selection must use the same item model."});
	m_Impl->m_Selection = std::move(selection);
	BindSelection(); InvalidatePaint(); return {};
}
void ListView::SetSelectionMode(SelectionModel::Mode mode) { m_Impl->m_Selection->SetMode(mode); }
Result<void> ListView::SetRootItem(ItemId item)
{
	if (item && (!m_Impl->m_Model || !m_Impl->m_Model->Contains(item))) return std::unexpected(Error{ErrorCode::InvalidArgument, "The root item does not exist."});
	Dispatch([&] { CancelEdit(); ++m_Impl->m_ModelGeneration; m_Impl->m_Root = item; RebuildRows(); SetOffset({}); });
	return {};
}
ItemId ListView::GetRootItem() const noexcept { return m_Impl->m_Root; }
void ListView::SetRowHeight(float height) { if (std::isfinite(height) && height >= 20) { m_Impl->m_RowHeight = height; InvalidateLayout(); } }
float ListView::GetRowHeight() const noexcept { return std::max(m_Impl->m_RowHeight, GetFontSize() * 1.5f + 8); }
std::size_t ListView::GetRowCount() const noexcept { return m_Impl->m_Rows.size(); }
std::size_t ListView::GetColumnCount() const noexcept { return !m_Impl->m_Model ? 0 : m_Impl->m_Kind == ViewKind::Table ? m_Impl->m_Model->GetColumnCount() : 1; }
std::size_t ListView::GetCurrentColumn() const noexcept { return m_Impl->m_CurrentColumn; }
Result<void> ListView::SetCurrentColumn(std::size_t column)
{
	if (column >= GetColumnCount()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid current column."});
	m_Impl->m_CurrentColumn = column;
	ScrollIntoView(m_Impl->m_Selection->GetCurrent());
	InvalidatePaint(); return {};
}
ItemId ListView::GetItemAt(std::size_t row) const noexcept { return row < m_Impl->m_Rows.size() ? m_Impl->m_Rows[row].m_Id : 0; }
std::optional<std::size_t> ListView::GetRowIndex(ItemId item) const
{
	const auto found = m_Impl->m_Indices.find(item);
	return found == m_Impl->m_Indices.end() ? std::nullopt : std::optional(found->second);
}
void ListView::RebuildRows()
{
	Detail::AccessibilityTree::StructureChanged(*this);
	m_Impl->m_Rows.clear(); m_Impl->m_Indices.clear();
	if (m_Impl->m_Model)
	{
		if (m_Impl->m_Root && !m_Impl->m_Model->Contains(m_Impl->m_Root)) m_Impl->m_Root = 0;
		struct Frame { ItemId m_Parent; std::size_t m_Row; std::size_t m_Count; unsigned m_Depth; };
		std::vector<Frame> stack{{m_Impl->m_Root, 0, m_Impl->m_Model->GetRowCount(m_Impl->m_Root), 0}};
		while (!stack.empty())
		{
			auto& frame = stack.back();
			if (frame.m_Row == frame.m_Count) { stack.pop_back(); continue; }
			const auto id = m_Impl->m_Model->GetChild(frame.m_Parent, frame.m_Row++);
			if (!id || m_Impl->m_Indices.contains(id)) continue;
			const auto children = m_Impl->m_Kind == ViewKind::Tree ? m_Impl->m_Model->GetRowCount(id) : 0;
			m_Impl->m_Indices.emplace(id, m_Impl->m_Rows.size());
			m_Impl->m_Rows.push_back({id, frame.m_Depth, children > 0, frame.m_Row - 1});
			if (children && m_Impl->m_Expanded.contains(id)) stack.push_back({id, 0, children, frame.m_Depth + 1});
		}
		m_Impl->m_Columns.resize(m_Impl->m_Model->GetColumnCount(), 180);
		m_Impl->m_CurrentColumn = std::min(m_Impl->m_CurrentColumn, GetColumnCount() ? GetColumnCount() - 1 : 0);
	}
	else m_Impl->m_Columns.clear();
	if (m_Impl->m_Anchor && (!m_Impl->m_Model || !m_Impl->m_Model->Contains(m_Impl->m_Anchor))) m_Impl->m_Anchor = 0;
	InvalidateLayout();
}
void ListView::ModelChanged(const ItemModel::Change& change)
{
	if (change.m_Kind == ItemModel::ChangeKind::Reset)
	{
		m_Impl->m_Expanded.clear(); m_Impl->m_Anchor = 0; CancelEdit();
	}
	else if (m_Impl->m_EditingItem && !m_Impl->m_Committing &&
		(!m_Impl->m_Model->Contains(m_Impl->m_EditingItem) || (change.m_Kind == ItemModel::ChangeKind::Updated &&
		std::ranges::find(change.m_Items, m_Impl->m_EditingItem) != change.m_Items.end()))) CancelEdit();
	if (change.m_Kind == ItemModel::ChangeKind::Removed) for (const auto item : change.m_Items) m_Impl->m_Expanded.erase(item);
	if (change.m_Kind != ItemModel::ChangeKind::Updated) RebuildRows();
	else InvalidatePaint();
}
bool ListView::IsExpanded(ItemId item) const { return m_Impl->m_Kind == ViewKind::Tree && m_Impl->m_Expanded.contains(item); }
void ListView::SetExpanded(ItemId item, bool expanded)
{
	if (m_Impl->m_Kind != ViewKind::Tree || !m_Impl->m_Model || !m_Impl->m_Model->Contains(item) || !m_Impl->m_Model->GetRowCount(item)) return;
	Dispatch([&]
	{
		if (expanded == IsExpanded(item)) return;
		if (expanded) m_Impl->m_Expanded.insert(item);
		else
		{
			m_Impl->m_Expanded.erase(item);
			for (auto parent = m_Impl->m_Model->GetParent(m_Impl->m_Selection->GetCurrent()); parent; parent = m_Impl->m_Model->GetParent(parent))
				if (parent == item) { m_Impl->m_Selection->SetCurrent(item); break; }
		}
		RebuildRows();
		if (m_Impl->m_EditingItem && !GetRowIndex(m_Impl->m_EditingItem)) CancelEdit();
		m_ExpandedChanged.Emit(item, expanded);
	});
}
Result<void> ListView::SetColumnWidthImpl(std::size_t column, float width)
{
	if (column >= GetColumnCount() || !std::isfinite(width) || width < 32 || width > 16384) return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid table column or width."});
	m_Impl->m_Columns[column] = width; InvalidateLayout(); return {};
}
float ListView::GetColumnWidthImpl(std::size_t column) const { return column < m_Impl->m_Columns.size() ? m_Impl->m_Columns[column] : 0; }
Result<void> ListView::SortImpl(std::size_t column, bool ascending)
{
	if (!m_Impl->m_Model) return std::unexpected(Error{ErrorCode::InvalidArgument, "A model is required."});
	Result<void> result;
	Dispatch([&] { result = m_Impl->m_Model->Sort(column, ascending); });
	return result;
}
Size ListView::OnMeasure(Size) { return {320, 220}; }
float ListView::HeaderHeight() const { return m_Impl->m_Kind == ViewKind::Table ? std::max(32.0f, GetTheme().m_ControlHeight) : 0; }
float ListView::ContentWidth() const
{
	if (m_Impl->m_Kind != ViewKind::Table) return m_Impl->m_Viewport.m_Width;
	float width = 0; for (const auto column : m_Impl->m_Columns) width += column; return width;
}
float ListView::ColumnLeft(std::size_t column) const
{
	float left = 0; for (std::size_t index = 0; index < column && index < m_Impl->m_Columns.size(); ++index) left += m_Impl->m_Columns[index]; return left;
}
void ListView::LayoutViewport()
{
	const auto size = GetActualSize();
	m_Impl->m_Viewport = {2, HeaderHeight() + 2, std::max(0.0f, size.m_Width - 4), std::max(0.0f, size.m_Height - HeaderHeight() - 4)};
	const float height = static_cast<float>(GetRowCount()) * GetRowHeight();
	if (height > m_Impl->m_Viewport.m_Height) m_Impl->m_Viewport.m_Width = std::max(0.0f, m_Impl->m_Viewport.m_Width - 12);
	if (ContentWidth() > m_Impl->m_Viewport.m_Width)
	{
		m_Impl->m_Viewport.m_Height = std::max(0.0f, m_Impl->m_Viewport.m_Height - 12);
		if (height > m_Impl->m_Viewport.m_Height) m_Impl->m_Viewport.m_Width = std::max(0.0f, size.m_Width - 16);
	}
	const auto previous = m_Impl->m_Offset;
	m_Impl->m_Offset = ClampOffset(previous); m_Impl->m_Target = ClampOffset(m_Impl->m_Target);
	if (previous.m_X != m_Impl->m_Offset.m_X) m_Impl->m_ScrollX.Snap(m_Impl->m_Offset.m_X);
	if (previous.m_Y != m_Impl->m_Offset.m_Y) m_Impl->m_ScrollY.Snap(m_Impl->m_Offset.m_Y);
}
void ListView::OnArrange(Rect) { LayoutViewport(); ArrangeEditor(); }
Rect ListView::GetChildrenClip() const noexcept
{
	const auto bounds = GetBounds(), viewport = m_Impl->m_Viewport;
	return {bounds.m_X + viewport.m_X, bounds.m_Y + viewport.m_Y, viewport.m_Width, viewport.m_Height};
}
Point ListView::ClampOffset(Point offset) const
{
	return {std::clamp(offset.m_X, 0.0f, std::max(0.0f, ContentWidth() - m_Impl->m_Viewport.m_Width)),
		std::clamp(offset.m_Y, 0.0f, std::max(0.0f, static_cast<float>(GetRowCount()) * GetRowHeight() - m_Impl->m_Viewport.m_Height))};
}
Point ListView::GetOffset() const noexcept { return m_Impl->m_Offset; }
void ListView::SetOffset(Point offset)
{
	if (!std::isfinite(offset.m_X) || !std::isfinite(offset.m_Y)) return;
	m_Impl->m_Offset = m_Impl->m_Target = ClampOffset(offset);
	m_Impl->m_ScrollX.Snap(m_Impl->m_Offset.m_X); m_Impl->m_ScrollY.Snap(m_Impl->m_Offset.m_Y);
	ArrangeEditor(); InvalidatePaint();
}
void ListView::ScrollTo(Point offset)
{
	m_Impl->m_Target = ClampOffset(offset);
	const auto duration = MotionDuration(GetTheme().m_Motion.m_ScrollDuration);
	m_Impl->m_ScrollX.Retarget(m_Impl->m_Target.m_X, duration); m_Impl->m_ScrollY.Retarget(m_Impl->m_Target.m_Y, duration);
	if (m_Impl->m_ScrollX.IsActive() || m_Impl->m_ScrollY.IsActive()) RequestAnimationFrame(); else SetOffset(m_Impl->m_Target);
}
bool ListView::OnAnimate(std::chrono::nanoseconds delta)
{
	const auto duration = MotionDuration(GetTheme().m_Motion.m_ScrollDuration);
	m_Impl->m_ScrollX.Retarget(m_Impl->m_Target.m_X, duration); m_Impl->m_ScrollY.Retarget(m_Impl->m_Target.m_Y, duration);
	bool active = m_Impl->m_ScrollX.Advance(delta); active = m_Impl->m_ScrollY.Advance(delta) || active;
	const Point offset{m_Impl->m_ScrollX.GetValue(), m_Impl->m_ScrollY.GetValue()};
	if (offset.m_X != m_Impl->m_Offset.m_X || offset.m_Y != m_Impl->m_Offset.m_Y) { m_Impl->m_Offset = offset; ArrangeEditor(); InvalidatePaint(); }
	return active;
}
std::pair<std::size_t, std::size_t> ListView::GetVisibleRange() const
{
	const auto first = static_cast<std::size_t>(std::max(0.0f, std::floor(m_Impl->m_Offset.m_Y / GetRowHeight())));
	const auto last = static_cast<std::size_t>(std::max(0.0f, std::ceil((m_Impl->m_Offset.m_Y + m_Impl->m_Viewport.m_Height) / GetRowHeight())));
	return {std::min(first, GetRowCount()), std::min(last, GetRowCount())};
}
Rect ListView::CellBounds(std::size_t row, std::size_t column) const
{
	const auto viewport = m_Impl->m_Viewport;
	return {viewport.m_X + ColumnLeft(column) - m_Impl->m_Offset.m_X, viewport.m_Y + static_cast<float>(row) * GetRowHeight() - m_Impl->m_Offset.m_Y,
		m_Impl->m_Kind == ViewKind::Table ? GetColumnWidthImpl(column) : viewport.m_Width, GetRowHeight()};
}
Rect ListView::GetItemBounds(ItemId item, std::size_t column) const
{
	const auto row = GetRowIndex(item);
	if (!row || column >= GetColumnCount()) return {};
	const auto local = CellBounds(*row, column).Intersect(m_Impl->m_Viewport), bounds = GetBounds();
	return local.Empty() ? Rect{} : Rect{bounds.m_X + local.m_X, bounds.m_Y + local.m_Y, local.m_Width, local.m_Height};
}
void ListView::ScrollIntoView(ItemId item)
{
	if (!m_Impl->m_Model || !m_Impl->m_Model->Contains(item)) return;
	if (m_Impl->m_Kind == ViewKind::Tree)
	{
		bool changed = false;
		for (auto parent = m_Impl->m_Model->GetParent(item); parent && parent != m_Impl->m_Root; parent = m_Impl->m_Model->GetParent(parent)) changed = m_Impl->m_Expanded.insert(parent).second || changed;
		if (changed) RebuildRows();
	}
	const auto row = GetRowIndex(item);
	if (!row) return;
	LayoutViewport();
	auto offset = m_Impl->m_Offset;
	const float y = static_cast<float>(*row) * GetRowHeight();
	if (y < offset.m_Y) offset.m_Y = y;
	else if (y + GetRowHeight() > offset.m_Y + m_Impl->m_Viewport.m_Height) offset.m_Y = y + GetRowHeight() - m_Impl->m_Viewport.m_Height;
	if (m_Impl->m_Kind == ViewKind::Table)
	{
		const float x = ColumnLeft(m_Impl->m_CurrentColumn), width = GetColumnWidthImpl(m_Impl->m_CurrentColumn);
		if (x < offset.m_X || width > m_Impl->m_Viewport.m_Width) offset.m_X = x;
		else if (x + width > offset.m_X + m_Impl->m_Viewport.m_Width) offset.m_X = x + width - m_Impl->m_Viewport.m_Width;
	}
	SetOffset(offset);
}
Rect ListView::GetFocusBounds() const noexcept
{
	const auto row = GetRowIndex(m_Impl->m_Selection->GetCurrent());
	if (!row) return {3, HeaderHeight() + 3, std::max(0.0f, GetActualSize().m_Width - 6), std::max(0.0f, GetActualSize().m_Height - HeaderHeight() - 6)};
	const auto rect = CellBounds(*row, m_Impl->m_CurrentColumn).Intersect(m_Impl->m_Viewport);
	return {rect.m_X + 1, rect.m_Y + 1, std::max(0.0f, rect.m_Width - 2), std::max(0.0f, rect.m_Height - 2)};
}
}
