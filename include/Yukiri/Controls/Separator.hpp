#pragma once
#include <Yukiri/Layout/StackPanel.hpp>

namespace Yukiri
{
class Separator : public Widget
{
public:
	explicit Separator(Orientation orientation = Orientation::Horizontal);
protected:
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
private:
	Orientation m_Orientation;
};
}
