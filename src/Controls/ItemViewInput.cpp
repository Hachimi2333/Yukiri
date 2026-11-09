#include "ItemViewState.hpp"
#include <cmath>

namespace Yukiri
{
std::optional<std::size_t> ListView::RowAt(Point local) const
{
	if (!m_Impl->m_Viewport.Contains(local)) return {};
	const auto row = static_cast<std::size_t>(std::floor((local.m_Y - m_Impl->m_Viewport.m_Y + m_Impl->m_Offset.m_Y) / GetRowHeight()));
	return row < GetRowCount() ? std::optional(row) : std::nullopt;
}
std::optional<std::size_t> ListView::ColumnAt(float x) const
{
	if (m_Impl->m_Kind != ViewKind::Table) return GetColumnCount() ? std::optional(std::size_t(0)) : std::nullopt;
	x += m_Impl->m_Offset.m_X - m_Impl->m_Viewport.m_X;
	for (std::size_t column = 0; column < GetColumnCount(); ++column)
	{
		if (x >= 0 && x < GetColumnWidthImpl(column)) return column;
		x -= GetColumnWidthImpl(column);
	}
	return {};
}
void ListView::SelectRow(std::size_t row, Modifiers modifiers)
{
	if (!m_Impl->m_Model || row >= GetRowCount()) return;
	const auto id = GetItemAt(row);
	const auto cell = m_Impl->m_Model->GetCell(id);
	if (!cell.m_Enabled || !cell.m_Selectable) return;
	auto& selection = *m_Impl->m_Selection;
	if (modifiers.m_Shift && selection.GetMode() == SelectionModel::Mode::Multiple)
	{
		if (!GetRowIndex(m_Impl->m_Anchor)) m_Impl->m_Anchor = GetRowIndex(selection.GetCurrent()) ? selection.GetCurrent() : id;
		const auto anchor = *GetRowIndex(m_Impl->m_Anchor);
		auto selected = modifiers.m_Control ? selection.GetSelected() : std::vector<ItemId>{};
		for (auto index = std::min(anchor, row); index <= std::max(anchor, row); ++index) selected.push_back(GetItemAt(index));
		selection.SetSelected(std::move(selected), id);
	}
	else { selection.Select(id, modifiers.m_Control, modifiers.m_Control); m_Impl->m_Anchor = id; }
	ScrollIntoView(id);
}
void ListView::MoveCurrent(int direction, bool edge, Modifiers modifiers)
{
	if (!GetRowCount() || !m_Impl->m_Model) return;
	const auto current = GetRowIndex(m_Impl->m_Selection->GetCurrent());
	int index = edge ? (direction < 0 ? 0 : static_cast<int>(GetRowCount()) - 1) : current ? static_cast<int>(*current) + direction : direction > 0 ? 0 : static_cast<int>(GetRowCount()) - 1;
	index = std::clamp(index, 0, static_cast<int>(GetRowCount()) - 1);
	const int step = direction < 0 ? -1 : 1;
	while (index >= 0 && index < static_cast<int>(GetRowCount()))
	{
		const auto id = GetItemAt(static_cast<std::size_t>(index));
		const auto cell = m_Impl->m_Model->GetCell(id);
		if (cell.m_Enabled && cell.m_Selectable)
		{
			if (modifiers.m_Control && !modifiers.m_Shift) { m_Impl->m_Selection->SetCurrent(id); ScrollIntoView(id); }
			else SelectRow(static_cast<std::size_t>(index), modifiers);
			return;
		}
		index += edge ? -step : step;
	}
}
void ListView::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left && event.m_Button != PointerButton::Right) return;
	const auto point = ToLocal(event.m_Position);
	const auto viewport = m_Impl->m_Viewport;
	if (event.m_Button == PointerButton::Left)
	{
		const auto vertical = VerticalThumb(), horizontal = HorizontalThumb();
		if (!vertical.Empty() && point.m_X >= vertical.m_X - 3 && point.m_Y >= viewport.m_Y)
		{
			m_Impl->m_DragAxis = 2;
			if (!vertical.Contains(point)) SetOffset({m_Impl->m_Offset.m_X, m_Impl->m_Offset.m_Y + (point.m_Y < vertical.m_Y ? -viewport.m_Height : viewport.m_Height)});
		}
		else if (!horizontal.Empty() && point.m_Y >= horizontal.m_Y - 3)
		{
			m_Impl->m_DragAxis = 1;
			if (!horizontal.Contains(point)) SetOffset({m_Impl->m_Offset.m_X + (point.m_X < horizontal.m_X ? -viewport.m_Width : viewport.m_Width), m_Impl->m_Offset.m_Y});
		}
		if (m_Impl->m_DragAxis)
		{
			SetOffset(m_Impl->m_Offset); m_Impl->m_DragStart = point; m_Impl->m_DragOffset = m_Impl->m_Offset;
			CapturePointer(); event.m_Handled = true; return;
		}
		if (point.m_Y < HeaderHeight() && m_Impl->m_Kind == ViewKind::Table)
		{
			for (std::size_t column = 0; column < GetColumnCount(); ++column)
				if (std::abs(point.m_X - (viewport.m_X + ColumnLeft(column) + GetColumnWidthImpl(column) - m_Impl->m_Offset.m_X)) < 4)
				{
					m_Impl->m_DragAxis = 3; m_Impl->m_ResizeColumn = column; m_Impl->m_ResizeWidth = GetColumnWidthImpl(column); m_Impl->m_DragStart = point;
					CapturePointer(); event.m_Handled = true; return;
				}
			if (const auto column = ColumnAt(point.m_X))
			{
				const auto sort = m_Impl->m_Model ? m_Impl->m_Model->GetSort() : std::nullopt;
				auto sorted = SortImpl(*column, !sort || sort->first != *column || !sort->second);
				if (!sorted) SetToolTip(sorted.error().m_Message);
			}
			event.m_Handled = true; return;
		}
	}
	const auto row = RowAt(point), column = ColumnAt(point.m_X);
	if (!row || !column || !m_Impl->m_Model) return;
	const auto entry = m_Impl->m_Rows[*row];
	const auto cell = m_Impl->m_Model->GetCell(entry.m_Id, *column);
	if (!cell.m_Enabled) return;
	m_Impl->m_CurrentColumn = *column;
	if (event.m_Button == PointerButton::Right)
	{
		if (!m_Impl->m_Selection->IsSelected(entry.m_Id)) SelectRow(*row, {});
		return;
	}
	float left = CellBounds(*row, *column).m_X + 8;
	if (m_Impl->m_Kind == ViewKind::Tree)
	{
		left += static_cast<float>(entry.m_Depth) * 18;
		if (entry.m_HasChildren && point.m_X >= left - 4 && point.m_X <= left + 14) { SetExpanded(entry.m_Id, !IsExpanded(entry.m_Id)); event.m_Handled = true; return; }
		left += 18;
	}
	SelectRow(*row, event.m_Modifiers);
	if (cell.m_Checked && point.m_X >= left && point.m_X < left + 20)
	{
		auto changed = m_Impl->m_Model->SetChecked(entry.m_Id, *column, !*cell.m_Checked);
		if (!changed) SetToolTip(changed.error().m_Message);
	}
	else if (event.m_ClickCount >= 2)
	{
		if (cell.m_Editable) { const auto editing = BeginEdit(entry.m_Id, *column); (void)editing; }
		else if (entry.m_HasChildren) SetExpanded(entry.m_Id, !IsExpanded(entry.m_Id));
		else m_ItemActivated.Emit(entry.m_Id);
	}
	else { m_Impl->m_DragSelecting = true; m_Impl->m_DragModifiers = event.m_Modifiers; CapturePointer(); }
	event.m_Handled = true;
}
void ListView::OnPointerUp(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left || (!m_Impl->m_DragAxis && !m_Impl->m_DragSelecting)) return;
	OnPointerCaptureLost(); ReleasePointer(); event.m_Handled = true;
}
void ListView::OnPointerCaptureLost() { m_Impl->m_DragAxis = 0; m_Impl->m_DragSelecting = false; SetCursor(Cursor::Arrow); }
void ListView::OnPointerMove(PointerEvent& event)
{
	const auto point = ToLocal(event.m_Position);
	const auto viewport = m_Impl->m_Viewport;
	if (m_Impl->m_DragAxis == 3)
	{
		auto resized = SetColumnWidthImpl(m_Impl->m_ResizeColumn, std::clamp(m_Impl->m_ResizeWidth + point.m_X - m_Impl->m_DragStart.m_X, 32.0f, 16384.0f)); (void)resized;
		event.m_Handled = true; return;
	}
	if (m_Impl->m_DragAxis)
	{
		if (m_Impl->m_DragAxis == 2) SetOffset({m_Impl->m_Offset.m_X, m_Impl->m_DragOffset.m_Y + (point.m_Y - m_Impl->m_DragStart.m_Y) * (static_cast<float>(GetRowCount()) * GetRowHeight() - viewport.m_Height) / std::max(1.0f, viewport.m_Height - VerticalThumb().m_Height)});
		else SetOffset({m_Impl->m_DragOffset.m_X + (point.m_X - m_Impl->m_DragStart.m_X) * (ContentWidth() - viewport.m_Width) / std::max(1.0f, viewport.m_Width - HorizontalThumb().m_Width), m_Impl->m_Offset.m_Y});
		event.m_Handled = true; return;
	}
	if (m_Impl->m_DragSelecting)
	{
		if (const auto row = RowAt(point); row && GetItemAt(*row) != m_Impl->m_Selection->GetCurrent()) { auto modifiers = m_Impl->m_DragModifiers; modifiers.m_Shift = true; SelectRow(*row, modifiers); }
		event.m_Handled = true; return;
	}
	bool resize = false;
	if (m_Impl->m_Kind == ViewKind::Table && point.m_Y < HeaderHeight()) for (std::size_t column = 0; column < GetColumnCount(); ++column)
		resize = resize || std::abs(point.m_X - (viewport.m_X + ColumnLeft(column) + GetColumnWidthImpl(column) - m_Impl->m_Offset.m_X)) < 4;
	SetCursor(resize ? Cursor::ResizeHorizontal : Cursor::Arrow);
}
void ListView::OnWheel(PointerEvent& event)
{
	const auto previous = m_Impl->m_Target;
	ScrollTo({previous.m_X - (event.m_Modifiers.m_Shift ? event.m_WheelY : event.m_WheelX) * 40, previous.m_Y - (event.m_Modifiers.m_Shift ? 0 : event.m_WheelY) * 40});
	event.m_Handled = previous.m_X != m_Impl->m_Target.m_X || previous.m_Y != m_Impl->m_Target.m_Y;
}
void ListView::OnKeyDown(KeyEvent& event)
{
	if (event.m_Modifiers.m_Alt || event.m_Modifiers.m_Super || !m_Impl->m_Model) return;
	if (event.m_Key == Key::Up || event.m_Key == Key::Down) MoveCurrent(event.m_Key == Key::Up ? -1 : 1, false, event.m_Modifiers);
	else if (event.m_Key == Key::Home || event.m_Key == Key::End) MoveCurrent(event.m_Key == Key::Home ? -1 : 1, true, event.m_Modifiers);
	else if (event.m_Key == Key::PageUp || event.m_Key == Key::PageDown) MoveCurrent((event.m_Key == Key::PageUp ? -1 : 1) * std::max(1, static_cast<int>(m_Impl->m_Viewport.m_Height / GetRowHeight()) - 1), false, event.m_Modifiers);
	else if (event.m_Key == Key::A && event.m_Modifiers.m_Control)
	{
		std::vector<ItemId> items; for (const auto& row : m_Impl->m_Rows) items.push_back(row.m_Id);
		m_Impl->m_Selection->SetSelected(std::move(items), m_Impl->m_Selection->GetCurrent());
	}
	else if (event.m_Key == Key::Escape && m_Impl->m_Editor) CancelEdit();
	else if (event.m_Key == Key::F2) { auto editing = BeginEdit(m_Impl->m_Selection->GetCurrent(), m_Impl->m_CurrentColumn); (void)editing; }
	else if (event.m_Key == Key::Enter) { if (const auto item = m_Impl->m_Selection->GetCurrent()) m_ItemActivated.Emit(item); }
	else if (event.m_Key == Key::Space)
	{
		const auto item = m_Impl->m_Selection->GetCurrent();
		if (item)
		{
			const auto cell = m_Impl->m_Model->GetCell(item, m_Impl->m_CurrentColumn);
			if (cell.m_Checked && !event.m_Modifiers.m_Control) { const auto changed = m_Impl->m_Model->SetChecked(item, m_Impl->m_CurrentColumn, !*cell.m_Checked); (void)changed; }
			else m_Impl->m_Selection->Select(item, true, true);
		}
	}
	else if (m_Impl->m_Kind == ViewKind::Tree && (event.m_Key == Key::Left || event.m_Key == Key::Right))
	{
		const auto item = m_Impl->m_Selection->GetCurrent();
		if (item && event.m_Key == Key::Right)
		{
			if (!IsExpanded(item)) SetExpanded(item, true);
			else MoveCurrent(1, false, event.m_Modifiers);
		}
		else if (item)
		{
			if (IsExpanded(item)) SetExpanded(item, false);
			else if (const auto row = GetRowIndex(m_Impl->m_Model->GetParent(item))) SelectRow(*row, event.m_Modifiers);
		}
	}
	else if ((event.m_Key == Key::Left || event.m_Key == Key::Right) && m_Impl->m_Kind == ViewKind::Table)
	{
		if (GetColumnCount())
		{
			const auto column = static_cast<std::size_t>(std::clamp(static_cast<int>(m_Impl->m_CurrentColumn) + (event.m_Key == Key::Left ? -1 : 1), 0, static_cast<int>(GetColumnCount()) - 1));
			const auto selected = SetCurrentColumn(column); (void)selected;
		}
	}
	else return;
	event.m_Handled = true;
}
}
