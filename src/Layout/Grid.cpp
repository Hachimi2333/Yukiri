#include <Yukiri/Layout/Grid.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace Yukiri
{
namespace
{
void Normalize(std::vector<GridLength>& tracks)
{
	if (tracks.empty()) tracks.push_back({});
	for (auto& track : tracks)
		if (!std::isfinite(track.m_Value) || track.m_Value < 0) track.m_Value = 0;
}
float Sum(const std::vector<float>& tracks, std::size_t first, std::size_t span, float spacing)
{
	float result = spacing * static_cast<float>(span - 1);
	for (std::size_t i = first; i < first + span; ++i) result += tracks[i];
	return result;
}
}
void Grid::SetRows(std::vector<GridLength> rows) { Normalize(rows); m_Rows = std::move(rows); InvalidateLayout(); }
void Grid::SetColumns(std::vector<GridLength> columns) { Normalize(columns); m_Columns = std::move(columns); InvalidateLayout(); }
void Grid::SetSpacing(float columns, float rows)
{
	m_ColumnSpacing = std::max(0.0f, columns);
	m_RowSpacing = std::max(0.0f, rows);
	InvalidateLayout();
}
Result<void> Grid::SetCell(Widget& child, std::size_t row, std::size_t column, std::size_t rowSpan, std::size_t columnSpan)
{
	if (child.GetParent() != this || !rowSpan || !columnSpan || row >= m_Rows.size() || column >= m_Columns.size() ||
		rowSpan > m_Rows.size() - row || columnSpan > m_Columns.size() - column)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "Grid cell or span is outside the configured tracks."});
	m_Cells[&child] = {row, column, rowSpan, columnSpan, child.m_Lifetime};
	InvalidateLayout();
	return {};
}
Grid::Cell Grid::GetCell(const Widget& child) const
{
	if (auto found = m_Cells.find(&child); found != m_Cells.end() && !found->second.m_Lifetime.expired())
	{
		auto cell = found->second;
		cell.m_Row = std::min(cell.m_Row, m_Rows.size() - 1);
		cell.m_Column = std::min(cell.m_Column, m_Columns.size() - 1);
		cell.m_RowSpan = std::min(cell.m_RowSpan, m_Rows.size() - cell.m_Row);
		cell.m_ColumnSpan = std::min(cell.m_ColumnSpan, m_Columns.size() - cell.m_Column);
		return cell;
	}
	return {};
}
std::vector<float> Grid::Resolve(const std::vector<GridLength>& tracks, bool horizontal,
	float available, float spacing, bool arrange) const
{
	std::vector<float> sizes(tracks.size());
	for (std::size_t i = 0; i < tracks.size(); ++i)
		if (tracks[i].m_Unit == GridUnit::Pixel) sizes[i] = tracks[i].m_Value;
	for (auto* child : Children())
	{
		if (!child->IsVisible()) continue;
		const auto cell = GetCell(*child);
		const auto first = horizontal ? cell.m_Column : cell.m_Row;
		const auto span = horizontal ? cell.m_ColumnSpan : cell.m_RowSpan;
		const auto desired = horizontal ? child->GetDesiredSize().m_Width : child->GetDesiredSize().m_Height;
		const auto deficit = std::max(0.0f, desired - Sum(sizes, first, span, spacing));
		std::vector<std::size_t> flexible;
		for (std::size_t i = first; i < first + span; ++i)
			if (tracks[i].m_Unit == GridUnit::Auto) flexible.push_back(i);
		if (flexible.empty())
			for (std::size_t i = first; i < first + span; ++i)
				if (tracks[i].m_Unit == GridUnit::Star) flexible.push_back(i);
		for (const auto index : flexible) sizes[index] += deficit / static_cast<float>(flexible.size());
	}
	if (arrange && std::isfinite(available))
	{
		float fixed = spacing * static_cast<float>(tracks.size() - 1);
		float weights = 0;
		for (std::size_t i = 0; i < tracks.size(); ++i)
			if (tracks[i].m_Unit == GridUnit::Star) weights += tracks[i].m_Value;
			else fixed += sizes[i];
		for (std::size_t i = 0; i < tracks.size(); ++i)
			if (tracks[i].m_Unit == GridUnit::Star)
				sizes[i] = weights > 0 ? std::max(0.0f, available - fixed) * tracks[i].m_Value / weights : 0;
	}
	return sizes;
}
Size Grid::OnMeasure(Size available)
{
	for (auto* child : Children()) child->Measure(available);
	const auto constrained = Resolve(m_Columns, true, available.m_Width, m_ColumnSpacing, true);
	for (auto* child : Children())
	{
		const auto cell = GetCell(*child);
		child->Measure({Sum(constrained, cell.m_Column, cell.m_ColumnSpan, m_ColumnSpacing), available.m_Height});
	}
	const auto columns = Resolve(m_Columns, true, available.m_Width, m_ColumnSpacing, false);
	const auto rows = Resolve(m_Rows, false, available.m_Height, m_RowSpacing, false);
	return {Sum(columns, 0, columns.size(), m_ColumnSpacing), Sum(rows, 0, rows.size(), m_RowSpacing)};
}
void Grid::OnArrange(Rect content)
{
	const auto columns = Resolve(m_Columns, true, content.m_Width, m_ColumnSpacing, true);
	for (auto* child : Children())
	{
		const auto cell = GetCell(*child);
		child->Measure({Sum(columns, cell.m_Column, cell.m_ColumnSpan, m_ColumnSpacing), content.m_Height});
	}
	const auto rows = Resolve(m_Rows, false, content.m_Height, m_RowSpacing, true);
	for (auto* child : Children())
	{
		const auto cell = GetCell(*child);
		float x = content.m_X;
		float y = content.m_Y;
		for (std::size_t i = 0; i < cell.m_Column; ++i) x += columns[i] + m_ColumnSpacing;
		for (std::size_t i = 0; i < cell.m_Row; ++i) y += rows[i] + m_RowSpacing;
		child->Arrange({x, y, Sum(columns, cell.m_Column, cell.m_ColumnSpan, m_ColumnSpacing),
			Sum(rows, cell.m_Row, cell.m_RowSpan, m_RowSpacing)});
	}
}
}
