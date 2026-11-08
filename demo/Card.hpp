#pragma once

#include <Yukiri/Layout/StackPanel.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace YukiriDemo
{
class Card : public Yukiri::StackPanel
{
public:
	Card() : StackPanel(Yukiri::Orientation::Vertical, 10) { SetPadding(Yukiri::Insets(20)); }
protected:
	void OnPaint(Yukiri::Canvas& canvas) override
	{
		canvas.FillRectangle(GetLocalBounds(), GetTheme().m_Surface, 10);
		canvas.StrokeRectangle(GetLocalBounds(), GetTheme().m_Border, 1, 10);
	}
};
}
