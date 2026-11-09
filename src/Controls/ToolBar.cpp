#include <Yukiri/Controls/ToolBar.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri
{
ToolBar::ToolBar() { SetPadding({8, 6, 8, 6}); SetSpacing(6, 6); }
void ToolBar::OnPaint(Canvas& canvas)
{
	canvas.FillRectangle(GetLocalBounds(), GetStyle().m_Background.value_or(GetTheme().m_Surface));
	canvas.FillRectangle({0, GetActualSize().m_Height - 1, GetActualSize().m_Width, 1}, GetTheme().m_Border);
}
}
