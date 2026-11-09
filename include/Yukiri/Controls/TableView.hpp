#pragma once

#include <Yukiri/Controls/ListView.hpp>

namespace Yukiri
{
class TableView : public ListView
{
public:
	TableView() : ListView(ViewKind::Table) {}
	[[nodiscard]] Result<void> SetColumnWidth(std::size_t column, float width) { return SetColumnWidthImpl(column, width); }
	[[nodiscard]] float GetColumnWidth(std::size_t column) const { return GetColumnWidthImpl(column); }
	[[nodiscard]] Result<void> Sort(std::size_t column, bool ascending = true) { return SortImpl(column, ascending); }
};
}
