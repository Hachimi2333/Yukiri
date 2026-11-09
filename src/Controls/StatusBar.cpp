#include <Yukiri/Controls/StatusBar.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri
{
StatusBar::StatusBar() : StackPanel(Orientation::Horizontal, 12) { SetPadding({12, 4, 12, 4}); }
void StatusBar::OnPaint(Canvas& canvas)
{
	canvas.FillRectangle(GetLocalBounds(), GetStyle().m_Background.value_or(GetTheme().m_Surface));
	canvas.FillRectangle({0, 0, GetActualSize().m_Width, 1}, GetTheme().m_Border);
}
}
