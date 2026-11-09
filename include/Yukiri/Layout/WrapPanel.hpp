#pragma once

#include <Yukiri/Layout/StackPanel.hpp>

namespace Yukiri
{
class WrapPanel : public Widget
{
public:
	explicit WrapPanel(Orientation orientation = Orientation::Horizontal);
	void SetOrientation(Orientation orientation);
	void SetSpacing(float horizontal, float vertical);
protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
private:
	Size Layout(Size available, Point origin, bool arrange);
	Orientation m_Orientation;
	float m_HorizontalSpacing = 8;
	float m_VerticalSpacing = 8;
};
}
