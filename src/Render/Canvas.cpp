#include <Yukiri/Render/Canvas.hpp>
#include "DrawList.hpp"
#include "Text/TextEngine.hpp"
#include <cassert>
#include <cmath>

namespace Yukiri
{
Canvas::Canvas(Detail::DrawList& list, Size size, Detail::TextEngine* text, float scale) : m_List(list), m_Text(text), m_Scale(scale)
{
	m_State.m_Clips.push_back({0, 0, size.m_Width, size.m_Height});
}

Rect Canvas::ToGlobal(Rect rectangle) const
{
	rectangle.m_X += m_State.m_Offset.m_X;
	rectangle.m_Y += m_State.m_Offset.m_Y;
	return rectangle;
}

void Canvas::FillRectangle(Rect rectangle, Color color, float radius)
{
	m_List.Add(ToGlobal(rectangle), color, m_State.m_Clips.back(), radius);
}

void Canvas::StrokeRectangle(Rect rectangle, Color color, float width, float radius)
{
	m_List.Add(ToGlobal(rectangle), color, m_State.m_Clips.back(), radius, width);
}

void Canvas::DrawLine(Point start, Point end, Color color, float width)
{
	const auto distance = std::hypot(end.m_X - start.m_X, end.m_Y - start.m_Y);
	const Rect rectangle{(start.m_X + end.m_X - distance - width) * 0.5f,
		(start.m_Y + end.m_Y - width) * 0.5f, distance + width, width};
	m_List.Add(ToGlobal(rectangle), color, m_State.m_Clips.back(), width * 0.5f, 0, 0, {0, 0, 1, 1}, false,
		std::atan2(end.m_Y - start.m_Y, end.m_X - start.m_X));
}

void Canvas::Save() { m_Stack.push_back(m_State); }
void Canvas::Restore()
{
	assert(!m_Stack.empty());
	m_State = std::move(m_Stack.back());
	m_Stack.pop_back();
}
void Canvas::SetOffset(Point offset) { m_State.m_Offset = offset; }
void Canvas::PushClip(Rect rectangle) { m_State.m_Clips.push_back(m_State.m_Clips.back().Intersect(ToGlobal(rectangle))); }
void Canvas::PopClip() { assert(m_State.m_Clips.size() > 1); m_State.m_Clips.pop_back(); }
void Canvas::DrawText(std::string_view text, Point position, Color color, float fontSize)
{
	if (!m_Text || text.empty() || m_State.m_Clips.back().Empty()) return;
	auto result = m_Text->Draw(m_List, text, position + m_State.m_Offset, color, fontSize, m_Scale, m_State.m_Clips.back());
	if (!result && !m_List.m_Error) m_List.m_Error = result.error();
}
Size Canvas::MeasureText(std::string_view text, float fontSize) const
{
	return m_Text ? m_Text->Measure(text, fontSize, m_Scale) : Size{};
}
}
