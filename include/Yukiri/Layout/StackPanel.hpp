#pragma once

#include <Yukiri/UI/Widget.hpp>

namespace Yukiri
{
enum class Orientation { Horizontal, Vertical };

class StackPanel : public Widget
{
public:
	explicit StackPanel(Orientation orientation = Orientation::Vertical, float spacing = 8);
	void SetOrientation(Orientation orientation);
	void SetSpacing(float spacing);

protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;

private:
	Orientation m_Orientation;
	float m_Spacing;
};
}
