#include <Yukiri/Layout/Border.hpp>
#include <Yukiri/Render/Canvas.hpp>

namespace Yukiri
{
Border::Border() { SetPadding(Insets(12)); }
Widget& Border::SetContent(std::unique_ptr<Widget> content) { ClearChildren(); return AddChild(std::move(content)); }
void Border::OnPaint(Canvas& canvas)
{
	canvas.FillRectangle(GetLocalBounds(), GetStyle().m_Background.value_or(GetTheme().m_Surface), GetRadius());
	canvas.StrokeRectangle(GetLocalBounds(), GetStyle().m_Border.value_or(GetTheme().m_Border), 1, GetRadius());
}
}
