#pragma once

#include <Yukiri/UI/Widget.hpp>

namespace YukiriDemo
{
class ResponsivePanel : public Yukiri::Widget
{
public:
	explicit ResponsivePanel(float columnWidth = 340, int maxColumns = 2, float spacing = 16);
protected:
	Yukiri::Size OnMeasure(Yukiri::Size available) override;
	void OnArrange(Yukiri::Rect content) override;
private:
	int Columns(float width) const;
	float m_ColumnWidth;
	int m_MaxColumns;
	float m_Spacing;
};
}
