#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/UI/Widget.hpp>
#include <unordered_map>

namespace Yukiri
{
enum class GridUnit { Auto, Pixel, Star };
struct GridLength
{
	GridUnit m_Unit = GridUnit::Star;
	float m_Value = 1;
	static GridLength Auto() noexcept { return {GridUnit::Auto, 0}; }
	static GridLength Pixels(float value) noexcept { return {GridUnit::Pixel, value}; }
	static GridLength Star(float weight = 1) noexcept { return {GridUnit::Star, weight}; }
};

class Grid : public Widget
{
public:
	void SetRows(std::vector<GridLength> rows);
	void SetColumns(std::vector<GridLength> columns);
	void SetSpacing(float columns, float rows);
	[[nodiscard]] Result<void> SetCell(Widget& child, std::size_t row, std::size_t column,
		std::size_t rowSpan = 1, std::size_t columnSpan = 1);

protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;

private:
	struct Cell
	{
		std::size_t m_Row = 0;
		std::size_t m_Column = 0;
		std::size_t m_RowSpan = 1;
		std::size_t m_ColumnSpan = 1;
		std::weak_ptr<int> m_Lifetime;
	};
	[[nodiscard]] Cell GetCell(const Widget& child) const;
	[[nodiscard]] std::vector<float> Resolve(const std::vector<GridLength>& tracks, bool horizontal,
		float available, float spacing, bool arrange) const;
	std::vector<GridLength> m_Rows{{}};
	std::vector<GridLength> m_Columns{{}};
	std::unordered_map<const Widget*, Cell> m_Cells;
	float m_ColumnSpacing = 8;
	float m_RowSpacing = 8;
};
}
