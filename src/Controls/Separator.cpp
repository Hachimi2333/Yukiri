#include <Yukiri/Controls/Separator.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri
{
Separator::Separator(Orientation orientation) : m_Orientation(orientation) { SetHitTestVisible(false); }
Size Separator::OnMeasure(Size) { return m_Orientation == Orientation::Horizontal ? Size{0, 1} : Size{1, 0}; }
void Separator::OnPaint(Canvas& canvas)
{
	const auto size = GetActualSize();
	canvas.FillRectangle(m_Orientation == Orientation::Horizontal ? Rect{0, size.m_Height * 0.5f, size.m_Width, 1} :
		Rect{size.m_Width * 0.5f, 0, 1, size.m_Height}, GetStyle().m_Foreground.value_or(GetTheme().m_Border));
}
}
