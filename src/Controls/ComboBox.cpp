#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/Unicode.hpp"
#include "PopupLayout.hpp"

namespace Yukiri
{
ComboBox::ComboBox(std::vector<std::string> items) { SetFocusable(true); SetCursor(Cursor::Hand); SetItems(std::move(items)); }
void ComboBox::SetItems(std::vector<std::string> items)
{
	ClosePopup();
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
	canvas.StrokeRectangle(GetLocalBounds(), HasFocus() ? theme.m_Accent : theme.m_Border, 1, GetRadius());
	canvas.PushClip({8, 0, std::max(0.0f, size.m_Width - 36), size.m_Height});
	canvas.DrawText(GetSelectedText(), {10, (size.m_Height - canvas.MeasureText(GetSelectedText(), GetFontSize()).m_Height) * 0.5f}, GetForeground(), GetFontSize());
	canvas.PopClip();
	const float x = size.m_Width - 18;
	const float y = size.m_Height * 0.5f;
	canvas.DrawLine({x - 4, y - 2}, {x, y + 2}, GetForeground(), 1.5f);
	canvas.DrawLine({x, y + 2}, {x + 4, y - 2}, GetForeground(), 1.5f);
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
	if (event.m_Key == Key::Space || event.m_Key == Key::Enter || event.m_Key == Key::Down)
	{
		event.m_Handled = true;
		Open();
	}
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
	ShowPopup([this](Canvas& canvas) { PaintPopup(canvas); }, [this](PointerEvent& event)
	{
		if (event.m_Button != PointerButton::Left) return false;
		if (m_PopupBounds.Contains(event.m_Position))
		{
			const int row = static_cast<int>((event.m_Position.m_Y - m_PopupBounds.m_Y - 4) / m_RowHeight) + m_TopIndex;
			ClosePopup();
			if (row >= 0 && row < static_cast<int>(m_Items.size())) { auto selected = SetSelectedIndex(row); (void)selected; }
			return true;
		}
		if (GetBounds().Contains(event.m_Position)) { ClosePopup(); return true; }
		return false;
	}, [this](KeyEvent& event)
	{
		if (event.m_Key == Key::Escape) { ClosePopup(); return true; }
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
		else return false;
		m_Highlight = std::clamp(m_Highlight, 0, static_cast<int>(m_Items.size()) - 1);
		m_TopIndex = std::clamp(m_TopIndex, std::max(0, m_Highlight - m_VisibleRows + 1), m_Highlight);
		InvalidatePaint();
		return true;
	});
}
void ComboBox::PaintPopup(Canvas& canvas)
{
	const auto& theme = GetTheme();
	canvas.FillRectangle(m_PopupBounds, theme.m_Surface, GetRadius());
	canvas.StrokeRectangle(m_PopupBounds, theme.m_Border, 1, GetRadius());
	canvas.PushClip(m_PopupBounds);
	for (int i = m_TopIndex; i < std::min(m_TopIndex + m_VisibleRows, static_cast<int>(m_Items.size())); ++i)
	{
		const float y = m_PopupBounds.m_Y + 4 + static_cast<float>(i - m_TopIndex) * m_RowHeight;
		if (i == m_Highlight) canvas.FillRectangle({m_PopupBounds.m_X + 4, y, m_PopupBounds.m_Width - 8, m_RowHeight}, theme.m_Hover, 4);
		canvas.DrawText(m_Items[static_cast<std::size_t>(i)], {m_PopupBounds.m_X + 12, y + 5}, theme.m_Text, GetFontSize());
	}
	canvas.PopClip();
}
}
