#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace YukiriDemo
{
class CanvasSample : public Yukiri::Widget
{
protected:
	Yukiri::Size OnMeasure(Yukiri::Size) override { return {260, 140}; }
	void OnPaint(Yukiri::Canvas& canvas) override
	{
		const auto& theme = GetTheme();
		canvas.FillRectangle({8, 8, 120, 72}, theme.m_Accent, 14);
		canvas.FillRectangle({80, 42, 120, 72}, Yukiri::Color::FromHex(0x36B98E, 0.72f), 18);
		canvas.StrokeRectangle({8, 8, 192, 106}, theme.m_Border, 1, 10);
		canvas.DrawLine({220, 20}, {260, 96}, theme.m_Accent, 3);
		canvas.DrawText("Canvas · rounded shapes & alpha", {8, 118}, theme.m_Text, 12);
	}
};
}
