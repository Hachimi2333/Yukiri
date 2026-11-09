#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/Unicode.hpp"
#include "PopupLayout.hpp"
#include <numbers>
#include <cmath>

namespace Yukiri
{
ComboBox::ComboBox(std::vector<std::string> items) { SetFocusable(true); SetCursor(Cursor::Hand); SetItems(std::move(items)); }
void ComboBox::SetItems(std::vector<std::string> items)
{
	ClosePopup(true);
	for (auto& item : items) item = Detail::SanitizeUtf8(item, true);
	m_Items = std::move(items);
	m_Selected = m_Items.empty() ? -1 : std::clamp(m_Selected, 0, static_cast<int>(m_Items.size()) - 1);
	InvalidateLayout();
}
Result<void> ComboBox::SetSelectedIndex(int index)
{
	if (index < -1 || index >= static_cast<int>(m_Items.size()))
		return std::unexpected(Error{ErrorCode::InvalidArgument, "ComboBox index is outside the item list."});
	if (m_Selected == index) return {};
	m_Selected = index;
	InvalidatePaint();
	m_SelectionChanged.Emit(index);
	return {};
}
int ComboBox::GetSelectedIndex() const noexcept { return m_Selected; }
std::string_view ComboBox::GetSelectedText() const noexcept { return m_Selected < 0 ? std::string_view{} : m_Items[static_cast<std::size_t>(m_Selected)]; }
Size ComboBox::OnMeasure(Size)
{
	float width = 140;
	for (const auto& item : m_Items) width = std::max(width, MeasureText(item).m_Width + 42);
	return {width, GetTheme().m_ControlHeight};
}
void ComboBox::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const auto size = GetActualSize();
	canvas.FillRectangle(GetLocalBounds(), InteractiveColor(GetStyle().m_Background.value_or(theme.m_Surface), theme.m_Hover, theme.m_Pressed, IsPopupOpen()), GetRadius());
	canvas.StrokeRectangle(GetLocalBounds(), IsFocusVisible() ? theme.m_Accent : theme.m_Border, 1, GetRadius());
	canvas.PushClip({8, 0, std::max(0.0f, size.m_Width - 36), size.m_Height});
	canvas.DrawText(GetSelectedText(), {10, (size.m_Height - canvas.MeasureText(GetSelectedText(), GetFontSize()).m_Height) * 0.5f}, GetForeground(), GetFontSize());
	canvas.PopClip();
	const float x = size.m_Width - 18;
	const float y = size.m_Height * 0.5f;
	const float angle = std::numbers::pi_v<float> * m_Arrow.GetValue();
	auto rotate = [x, y, angle](Point point) { return Point{x + point.m_X * std::cos(angle) - point.m_Y * std::sin(angle),
		y + point.m_X * std::sin(angle) + point.m_Y * std::cos(angle)}; };
	canvas.DrawLine(rotate({-4, -2}), rotate({0, 2}), GetForeground(), 1.5f);
	canvas.DrawLine(rotate({0, 2}), rotate({4, -2}), GetForeground(), 1.5f);
	PaintFocus(canvas);
}
void ComboBox::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	event.m_Handled = true;
	if (IsPopupOpen()) ClosePopup(); else Open();
}
void ComboBox::OnKeyDown(KeyEvent& event)
{
	if (event.m_Modifiers.m_Control || event.m_Modifiers.m_Super || (event.m_Modifiers.m_Alt && event.m_Key != Key::Down)) return;
	if (event.m_Key == Key::Space || event.m_Key == Key::Enter || event.m_Key == Key::F4 || (event.m_Key == Key::Down && event.m_Modifiers.m_Alt))
	{
		event.m_Handled = true;
		Open();
	}
	else if (!m_Items.empty() && (event.m_Key == Key::Up || event.m_Key == Key::Down || event.m_Key == Key::Home || event.m_Key == Key::End))
	{
		const int index = event.m_Key == Key::Home ? 0 : event.m_Key == Key::End ? static_cast<int>(m_Items.size()) - 1 :
			std::clamp(m_Selected + (event.m_Key == Key::Up ? -1 : 1), 0, static_cast<int>(m_Items.size()) - 1);
		auto selected = SetSelectedIndex(index); (void)selected;
		event.m_Handled = true;
	}
}
const std::vector<std::string>& ComboBox::GetItems() const noexcept { return m_Items; }
bool ComboBox::IsExpanded() const noexcept { return IsPopupOpen(); }
void ComboBox::SetExpanded(bool expanded) { if (expanded && !IsPopupOpen()) Open(); else if (!expanded) ClosePopup(); }
bool ComboBox::OnAnimate(std::chrono::nanoseconds delta)
{
	m_Arrow.Retarget(IsPopupOpen() ? 1.0f : 0.0f, MotionDuration(GetTheme().m_Motion.m_PopupDuration));
	m_HighlightVisual.Retarget(static_cast<float>(m_Highlight - m_TopIndex), MotionDuration(GetTheme().m_Motion.m_StateDuration));
	bool active = m_Arrow.Advance(delta);
	active = m_HighlightVisual.Advance(delta) || active;
	return active;
}
Rect ComboBox::VisiblePopup() const
{
	const float height = m_PopupBounds.m_Height * m_VisualProgress;
	return {m_PopupBounds.m_X, m_PopupUp ? m_PopupBounds.Bottom() - height : m_PopupBounds.m_Y, m_PopupBounds.m_Width, height};
}
void ComboBox::Open()
{
	if (m_Items.empty()) return;
	const auto bounds = GetBounds();
	const auto viewport = GetWindow() ? GetWindow()->GetContentBounds() : Rect{0, 0, 1920, 1080};
	m_RowHeight = std::max(32.0f, GetFontSize() * 1.5f + 10);
	const auto placement = Detail::PlacePopup(bounds, viewport, static_cast<int>(m_Items.size()), m_RowHeight);
	if (!placement.m_VisibleRows) return;
	m_PopupBounds = placement.m_Bounds;
	m_VisibleRows = placement.m_VisibleRows;
	m_Highlight = std::max(0, m_Selected);
	m_TopIndex = std::max(0, m_Highlight - m_VisibleRows + 1);
	m_HighlightVisual.Snap(static_cast<float>(m_Highlight - m_TopIndex));
	m_PopupUp = m_PopupBounds.Bottom() <= bounds.m_Y;
	m_VisualProgress = MotionDuration(GetTheme().m_Motion.m_PopupDuration).count() ? 0.0f : 1.0f;
	m_WheelRemainder = 0;
	RequestAnimationFrame();
	ShowPopup([this](Canvas& canvas, float progress) { PaintPopup(canvas, progress); }, [this](PointerEvent& event)
	{
		if (VisiblePopup().Contains(event.m_Position))
		{
			if (event.m_WheelY != 0)
			{
				m_WheelRemainder -= event.m_WheelY;
				const int rows = static_cast<int>(m_WheelRemainder);
				m_WheelRemainder -= static_cast<float>(rows);
				m_TopIndex = std::clamp(m_TopIndex + rows, 0, std::max(0, static_cast<int>(m_Items.size()) - m_VisibleRows));
				m_Highlight = std::clamp(m_Highlight, m_TopIndex, m_TopIndex + m_VisibleRows - 1);
				m_HighlightVisual.Snap(static_cast<float>(m_Highlight - m_TopIndex));
				InvalidatePaint();
				return true;
			}
			const float relative = event.m_Position.m_Y - m_PopupBounds.m_Y - 4;
			const int row = static_cast<int>(std::floor(relative / m_RowHeight)) + m_TopIndex;
			if (relative < 0 || row >= std::min(m_TopIndex + m_VisibleRows, static_cast<int>(m_Items.size()))) return true;
			if (event.m_Button == PointerButton::None)
			{
				if (m_Highlight != row) { m_Highlight = row; RequestAnimationFrame(); }
				return true;
			}
			if (event.m_Button == PointerButton::Left)
			{
				ClosePopup();
				auto selected = SetSelectedIndex(row);
				(void)selected;
				return true;
			}
			return false;
		}
		if (event.m_Button == PointerButton::Left && GetBounds().Contains(event.m_Position)) { ClosePopup(); return true; }
		return false;
	}, [this](KeyEvent& event)
	{
		if (event.m_Key == Key::Escape || event.m_Key == Key::F4 || (event.m_Key == Key::Up && event.m_Modifiers.m_Alt)) { ClosePopup(); return true; }
		if (event.m_Key == Key::Enter || event.m_Key == Key::Space)
		{
			const int selectedIndex = m_Highlight;
			ClosePopup();
			auto selected = SetSelectedIndex(selectedIndex);
			(void)selected;
			return true;
		}
		if (event.m_Key == Key::Down) ++m_Highlight;
		else if (event.m_Key == Key::Up) --m_Highlight;
		else if (event.m_Key == Key::Home) m_Highlight = 0;
		else if (event.m_Key == Key::End) m_Highlight = static_cast<int>(m_Items.size()) - 1;
		else if (event.m_Key == Key::PageUp) m_Highlight -= m_VisibleRows;
		else if (event.m_Key == Key::PageDown) m_Highlight += m_VisibleRows;
		else return false;
		m_Highlight = std::clamp(m_Highlight, 0, static_cast<int>(m_Items.size()) - 1);
		m_TopIndex = std::clamp(m_TopIndex, std::max(0, m_Highlight - m_VisibleRows + 1), m_Highlight);
		RequestAnimationFrame();
		InvalidatePaint();
		return true;
	});
}
void ComboBox::PaintPopup(Canvas& canvas, float progress)
{
	m_VisualProgress = progress;
	const auto& theme = GetTheme();
	canvas.PushClip(VisiblePopup());
	canvas.FillRectangle(m_PopupBounds, theme.m_Surface.WithAlpha(theme.m_Surface.m_A * progress), GetRadius());
	canvas.StrokeRectangle(m_PopupBounds, theme.m_Border.WithAlpha(theme.m_Border.m_A * progress), 1, GetRadius());
	canvas.FillRectangle({m_PopupBounds.m_X + 4, m_PopupBounds.m_Y + 4 + m_HighlightVisual.GetValue() * m_RowHeight,
		m_PopupBounds.m_Width - 8, m_RowHeight}, theme.m_Hover.WithAlpha(progress), 4);
	for (int i = m_TopIndex; i < std::min(m_TopIndex + m_VisibleRows, static_cast<int>(m_Items.size())); ++i)
	{
		const float y = m_PopupBounds.m_Y + 4 + static_cast<float>(i - m_TopIndex) * m_RowHeight;
		canvas.DrawText(m_Items[static_cast<std::size_t>(i)], {m_PopupBounds.m_X + 12, y + 5}, theme.m_Text.WithAlpha(theme.m_Text.m_A * progress), GetFontSize());
	}
	canvas.PopClip();
}
}
