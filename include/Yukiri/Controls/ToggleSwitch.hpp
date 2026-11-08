#pragma once

#include <Yukiri/Controls/CheckBox.hpp>

namespace Yukiri
{
class ToggleSwitch : public CheckBox
{
public:
	using CheckBox::CheckBox;
protected:
	Rect GetFocusBounds() const noexcept override;
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
};
}
