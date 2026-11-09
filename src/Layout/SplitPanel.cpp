#include <Yukiri/Layout/SplitPanel.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <algorithm>
#include <cmath>
#include <cassert>

namespace Yukiri
{
namespace { constexpr float DividerWidth = 6; }
SplitPanel::SplitPanel(Orientation orientation) : m_Orientation(orientation)
{
	SetFocusable(true);
	SetCursor(orientation == Orientation::Horizontal ? Cursor::ResizeHorizontal : Cursor::ResizeVertical);
}
void SplitPanel::SetContent(std::unique_ptr<Widget> first, std::unique_ptr<Widget> second)
{
	ClearChildren();
	assert(first && second);
	AddChild(std::move(first));
	AddChild(std::move(second));
}
void SplitPanel::SetRatio(float ratio)
{
	if (!std::isfinite(ratio)) return;
	ratio = std::clamp(ratio, 0.0f, 1.0f);
	if (m_Ratio == ratio) return;
	m_Ratio = ratio;
	InvalidateLayout();
	m_RatioChanged.Emit(ratio);
}
float SplitPanel::GetRatio() const noexcept { return m_Ratio; }
void SplitPanel::SetMinimumSizes(float first, float second)
{
	m_FirstMinimum = std::isfinite(first) ? std::max(0.0f, first) : 0;
	m_SecondMinimum = std::isfinite(second) ? std::max(0.0f, second) : 0;
	InvalidateLayout();
}
float SplitPanel::FirstSize(float available) const
{
	if (!std::isfinite(available)) return 0;
	available = std::max(0.0f, available);
	const float minimum = m_FirstMinimum + m_SecondMinimum;
	if (minimum > available) return minimum > 0 ? available * m_FirstMinimum / minimum : 0;
	return std::clamp(available * m_Ratio, m_FirstMinimum, available - m_SecondMinimum);
}
Rect SplitPanel::Content() const noexcept
{
	const auto bounds = GetBounds();
	const auto padding = GetPadding();
	return {bounds.m_X + padding.m_Left, bounds.m_Y + padding.m_Top, std::max(0.0f, bounds.m_Width - padding.Horizontal()),
		std::max(0.0f, bounds.m_Height - padding.Vertical())};
}
Size SplitPanel::OnMeasure(Size available)
{
	const bool horizontal = m_Orientation == Orientation::Horizontal;
	const float main = horizontal ? available.m_Width : available.m_Height;
	const float remaining = std::max(0.0f, main - DividerWidth);
	const float first = FirstSize(remaining);
	Size desired;
	std::size_t index = 0;
	for (auto* child : Children())
	{
		if (index >= 2) break;
		const float length = std::isfinite(main) ? (index == 0 ? first : remaining - first) : main;
		child->Measure(horizontal ? Size{length, available.m_Height} : Size{available.m_Width, length});
		const auto size = child->GetDesiredSize();
		if (horizontal) { desired.m_Width += size.m_Width; desired.m_Height = std::max(desired.m_Height, size.m_Height); }
		else { desired.m_Height += size.m_Height; desired.m_Width = std::max(desired.m_Width, size.m_Width); }
		++index;
	}
	if (horizontal) desired.m_Width += DividerWidth; else desired.m_Height += DividerWidth;
	return desired;
}
void SplitPanel::OnArrange(Rect content)
{
	const bool horizontal = m_Orientation == Orientation::Horizontal;
	const float main = horizontal ? content.m_Width : content.m_Height;
	const float divider = std::min(DividerWidth, main);
	const float available = std::max(0.0f, main - divider);
	const float first = FirstSize(available);
	const auto dividerBounds = horizontal ? Rect{content.m_X + first, content.m_Y, divider, content.m_Height} :
		Rect{content.m_X, content.m_Y + first, content.m_Width, divider};
	const auto bounds = GetBounds();
	m_Divider = {dividerBounds.m_X - bounds.m_X, dividerBounds.m_Y - bounds.m_Y, dividerBounds.m_Width, dividerBounds.m_Height};
	const Rect slots[] = {
		horizontal ? Rect{content.m_X, content.m_Y, first, content.m_Height} : Rect{content.m_X, content.m_Y, content.m_Width, first},
		horizontal ? Rect{dividerBounds.Right(), content.m_Y, available - first, content.m_Height} : Rect{content.m_X, dividerBounds.Bottom(), content.m_Width, available - first}
	};
	std::size_t index = 0;
	for (auto* child : Children())
	{
		if (index >= 2) break;
		child->Measure({slots[index].m_Width, slots[index].m_Height});
		child->Arrange(slots[index++]);
	}
}
void SplitPanel::OnPointerDown(PointerEvent& event)
{
	const auto local = ToLocal(event.m_Position);
	if (event.m_Button != PointerButton::Left || !m_Divider.Contains(local)) return;
	m_Dragging = true;
	m_DragOffset = m_Orientation == Orientation::Horizontal ? local.m_X - m_Divider.m_X : local.m_Y - m_Divider.m_Y;
	CapturePointer();
	event.m_Handled = true;
	InvalidatePaint();
}
void SplitPanel::OnPointerMove(PointerEvent& event)
{
	if (!m_Dragging) return;
	const auto content = Content();
	const bool horizontal = m_Orientation == Orientation::Horizontal;
	const float available = std::max(0.0f, (horizontal ? content.m_Width : content.m_Height) - DividerWidth);
	if (available <= 0) return;
	const float first = (horizontal ? event.m_Position.m_X - content.m_X : event.m_Position.m_Y - content.m_Y) - m_DragOffset;
	event.m_Handled = true;
	const float minimum = m_FirstMinimum + m_SecondMinimum > available ? 0 : m_FirstMinimum;
	const float maximum = m_FirstMinimum + m_SecondMinimum > available ? available : available - m_SecondMinimum;
	SetRatio(std::clamp(first, minimum, maximum) / available);
}
void SplitPanel::OnPointerUp(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left || !m_Dragging) return;
	event.m_Handled = true;
	m_Dragging = false;
	ReleasePointer();
}
void SplitPanel::OnPointerCaptureLost() { m_Dragging = false; InvalidatePaint(); }
void SplitPanel::OnFocusChanged(bool focused) { if (!focused) { m_Dragging = false; ReleasePointer(); } InvalidatePaint(); }
void SplitPanel::OnKeyDown(KeyEvent& event)
{
	const bool horizontal = m_Orientation == Orientation::Horizontal;
	const auto content = Content();
	const float available = std::max(1.0f, (horizontal ? content.m_Width : content.m_Height) - DividerWidth);
	float ratio = FirstSize(available) / available;
	const float step = (event.m_Modifiers.m_Shift ? 1.0f : 10.0f) / available;
	if (event.m_Key == (horizontal ? Key::Left : Key::Up)) ratio -= step;
	else if (event.m_Key == (horizontal ? Key::Right : Key::Down)) ratio += step;
	else if (event.m_Key == Key::Home) ratio = 0;
	else if (event.m_Key == Key::End) ratio = 1;
	else return;
	event.m_Handled = true;
	SetRatio(ratio);
}
Rect SplitPanel::GetFocusBounds() const noexcept
{
	return m_Divider;
}
void SplitPanel::OnPaintOverlay(Canvas& canvas)
{
	auto rectangle = GetFocusBounds();
	canvas.FillRectangle(rectangle, InteractiveColor(GetTheme().m_Border, GetTheme().m_Accent, GetTheme().m_Accent, m_Dragging), 2);
	PaintFocus(canvas);
}
}
