#include <Yukiri/Controls/ScrollView.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "UI/WidgetTree.hpp"
#include <limits>
#include <cmath>

namespace Yukiri
{
ScrollView::ScrollView() { SetFocusable(true); }
Widget& ScrollView::SetContent(std::unique_ptr<Widget> content) { SetOffset({}); ClearChildren(); return AddChild(std::move(content)); }
void ScrollView::SetHorizontalScrollEnabled(bool enabled) { m_Horizontal = enabled; InvalidateLayout(); }
void ScrollView::SetVerticalScrollEnabled(bool enabled) { m_Vertical = enabled; InvalidateLayout(); }
Point ScrollView::GetOffset() const noexcept { return m_Offset; }
Point ScrollView::ClampOffset(Point offset) const
{
	return {m_Horizontal ? std::clamp(offset.m_X, 0.0f, std::max(0.0f, m_Content.m_Width - m_Viewport.m_Width)) : 0,
		m_Vertical ? std::clamp(offset.m_Y, 0.0f, std::max(0.0f, m_Content.m_Height - m_Viewport.m_Height)) : 0};
}
void ScrollView::ApplyOffset(Point offset)
{
	offset = ClampOffset(offset);
	const auto delta = m_Offset - offset;
	if (delta.m_X == 0 && delta.m_Y == 0) return;
	m_Offset = offset;
	for (const auto& child : GetChildren()) child->TranslateSubtree(delta);
	if (m_Tree) m_Tree->ClosePopup(true);
	InvalidatePaint();
}
void ScrollView::SetOffset(Point offset)
{
	if (!std::isfinite(offset.m_X) || !std::isfinite(offset.m_Y)) return;
	m_Target = ClampOffset(offset);
	m_ScrollX.Snap(m_Target.m_X);
	m_ScrollY.Snap(m_Target.m_Y);
	ApplyOffset(m_Target);
}
void ScrollView::SetSmoothScrolling(bool enabled)
{
	m_Smooth = enabled;
	if (!enabled) SetOffset(m_Target);
}
void ScrollView::ScrollTo(Point target)
{
	m_Target = ClampOffset(target);
	const auto duration = m_Smooth ? MotionDuration(GetTheme().m_Motion.m_ScrollDuration) : std::chrono::nanoseconds::zero();
	m_ScrollX.Retarget(m_Target.m_X, duration);
	m_ScrollY.Retarget(m_Target.m_Y, duration);
	if (m_ScrollX.IsActive() || m_ScrollY.IsActive()) RequestAnimationFrame();
	else ApplyOffset(m_Target);
}
bool ScrollView::OnAnimate(std::chrono::nanoseconds delta)
{
	const auto duration = m_Smooth ? MotionDuration(GetTheme().m_Motion.m_ScrollDuration) : std::chrono::nanoseconds::zero();
	m_ScrollX.Retarget(m_Target.m_X, duration);
	m_ScrollY.Retarget(m_Target.m_Y, duration);
	bool active = m_ScrollX.Advance(delta);
	active = m_ScrollY.Advance(delta) || active;
	ApplyOffset({m_ScrollX.GetValue(), m_ScrollY.GetValue()});
	return active;
}
Size ScrollView::OnMeasure(Size available)
{
	if (GetChildren().empty()) return {};
	const float infinity = std::numeric_limits<float>::infinity();
	GetChildren().front()->Measure({m_Horizontal ? infinity : available.m_Width, m_Vertical ? infinity : available.m_Height});
	const auto desired = GetChildren().front()->GetDesiredSize();
	return {std::min(desired.m_Width, available.m_Width), std::min(desired.m_Height, available.m_Height)};
}
void ScrollView::OnArrange(Rect content)
{
	m_Viewport = {content.m_Width, content.m_Height};
	if (GetChildren().empty()) { m_Content = {}; return; }
	auto& child = *GetChildren().front();
	m_Content = child.GetDesiredSize();
	if (m_Vertical && m_Content.m_Height > m_Viewport.m_Height) m_Viewport.m_Width = std::max(0.0f, m_Viewport.m_Width - 12);
	if (m_Horizontal && m_Content.m_Width > m_Viewport.m_Width) m_Viewport.m_Height = std::max(0.0f, m_Viewport.m_Height - 12);
	child.Measure({m_Horizontal ? std::numeric_limits<float>::infinity() : m_Viewport.m_Width,
		m_Vertical ? std::numeric_limits<float>::infinity() : m_Viewport.m_Height});
	m_Content = child.GetDesiredSize();
	m_Content.m_Width = std::max(m_Content.m_Width, m_Viewport.m_Width);
	m_Content.m_Height = std::max(m_Content.m_Height, m_Viewport.m_Height);
	const auto target = ClampOffset(m_Target);
	const auto offset = ClampOffset(m_Offset);
	if (offset.m_X != m_Offset.m_X || offset.m_Y != m_Offset.m_Y)
	{
		m_ScrollX.Snap(offset.m_X);
		m_ScrollY.Snap(offset.m_Y);
	}
	ApplyOffset(offset);
	ScrollTo(target);
	child.Arrange({content.m_X - m_Offset.m_X, content.m_Y - m_Offset.m_Y, m_Content.m_Width, m_Content.m_Height});
}
Rect ScrollView::VerticalThumb() const
{
	if (!m_Vertical || m_Content.m_Height <= m_Viewport.m_Height || m_Viewport.m_Height <= 0) return {};
	const float height = std::min(m_Viewport.m_Height, std::max(24.0f, m_Viewport.m_Height * m_Viewport.m_Height / m_Content.m_Height));
	return {GetActualSize().m_Width - 9, (m_Viewport.m_Height - height) * m_Offset.m_Y /
		(m_Content.m_Height - m_Viewport.m_Height), 6, height};
}
Rect ScrollView::HorizontalThumb() const
{
	if (!m_Horizontal || m_Content.m_Width <= m_Viewport.m_Width || m_Viewport.m_Width <= 0) return {};
	const float width = std::min(m_Viewport.m_Width, std::max(24.0f, m_Viewport.m_Width * m_Viewport.m_Width / m_Content.m_Width));
	return {(m_Viewport.m_Width - width) * m_Offset.m_X / (m_Content.m_Width - m_Viewport.m_Width),
		GetActualSize().m_Height - 9, width, 6};
}
void ScrollView::OnPaintOverlay(Canvas& canvas)
{
	const auto color = InteractiveColor(GetTheme().m_MutedText.WithAlpha(0.55f), GetTheme().m_MutedText.WithAlpha(0.75f),
		GetTheme().m_MutedText.WithAlpha(0.95f), m_DragAxis != 0);
	canvas.FillRectangle(VerticalThumb(), color, 3);
	canvas.FillRectangle(HorizontalThumb(), color, 3);
	PaintFocus(canvas);
}
void ScrollView::OnWheel(PointerEvent& event)
{
	const auto previous = m_Target;
	ScrollTo({m_Target.m_X - (event.m_Modifiers.m_Shift ? event.m_WheelY : event.m_WheelX) * 40,
		m_Target.m_Y - (event.m_Modifiers.m_Shift ? 0 : event.m_WheelY) * 40});
	event.m_Handled = previous.m_X != m_Target.m_X || previous.m_Y != m_Target.m_Y;
}
void ScrollView::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	const auto local = ToLocal(event.m_Position);
	const auto vertical = VerticalThumb();
	const auto horizontal = HorizontalThumb();
	if (!vertical.Empty() && local.m_X >= GetActualSize().m_Width - 12)
	{
		m_DragAxis = 2;
		if (!vertical.Contains(local)) SetOffset({m_Offset.m_X, m_Offset.m_Y + (local.m_Y < vertical.m_Y ? -m_Viewport.m_Height : m_Viewport.m_Height)});
	}
	else if (!horizontal.Empty() && local.m_Y >= GetActualSize().m_Height - 12)
	{
		m_DragAxis = 1;
		if (!horizontal.Contains(local)) SetOffset({m_Offset.m_X + (local.m_X < horizontal.m_X ? -m_Viewport.m_Width : m_Viewport.m_Width), m_Offset.m_Y});
	}
	else return;
	SetOffset(m_Offset);
	m_DragStart = event.m_Position;
	m_DragOffset = m_Offset;
	CapturePointer();
	event.m_Handled = true;
}
void ScrollView::OnPointerMove(PointerEvent& event)
{
	if (!m_DragAxis) return;
	if (m_DragAxis == 2)
		SetOffset({m_Offset.m_X, m_DragOffset.m_Y + (event.m_Position.m_Y - m_DragStart.m_Y) *
			(m_Content.m_Height - m_Viewport.m_Height) / std::max(1.0f, m_Viewport.m_Height - VerticalThumb().m_Height)});
	else
		SetOffset({m_DragOffset.m_X + (event.m_Position.m_X - m_DragStart.m_X) *
			(m_Content.m_Width - m_Viewport.m_Width) / std::max(1.0f, m_Viewport.m_Width - HorizontalThumb().m_Width), m_Offset.m_Y});
	event.m_Handled = true;
}
void ScrollView::OnPointerUp(PointerEvent& event)
{
	if (event.m_Button == PointerButton::Left && m_DragAxis)
	{
		m_DragAxis = 0;
		ReleasePointer();
		event.m_Handled = true;
	}
}
void ScrollView::OnFocusChanged(bool focused) { if (!focused) { m_DragAxis = 0; ReleasePointer(); } InvalidatePaint(); }
void ScrollView::OnKeyDown(KeyEvent& event)
{
	Point offset = m_Target;
	if (event.m_Key == Key::Down) offset.m_Y += 40;
	else if (event.m_Key == Key::Up) offset.m_Y -= 40;
	else if (event.m_Key == Key::Home) offset.m_Y = 0;
	else if (event.m_Key == Key::End) offset.m_Y = m_Content.m_Height;
	else return;
	ScrollTo(offset);
	event.m_Handled = true;
}
void ScrollView::OnBringIntoView(Rect& rectangle)
{
	const auto bounds = GetBounds();
	Point target = m_Offset;
	if (rectangle.m_Y < bounds.m_Y) target.m_Y -= bounds.m_Y - rectangle.m_Y;
	else if (rectangle.Bottom() > bounds.m_Y + m_Viewport.m_Height) target.m_Y += rectangle.Bottom() - bounds.m_Y - m_Viewport.m_Height;
	if (rectangle.m_X < bounds.m_X) target.m_X -= bounds.m_X - rectangle.m_X;
	else if (rectangle.Right() > bounds.m_X + m_Viewport.m_Width) target.m_X += rectangle.Right() - bounds.m_X - m_Viewport.m_Width;
	const auto old = m_Offset;
	SetOffset(target);
	rectangle.m_X -= m_Offset.m_X - old.m_X;
	rectangle.m_Y -= m_Offset.m_Y - old.m_Y;
}
Rect ScrollView::GetChildrenClip() const noexcept
{
	const auto bounds = GetBounds();
	const auto padding = GetPadding();
	return {bounds.m_X + padding.m_Left, bounds.m_Y + padding.m_Top, m_Viewport.m_Width, m_Viewport.m_Height};
}
}
