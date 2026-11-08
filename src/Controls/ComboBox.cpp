#include <Yukiri/Controls/ComboBox.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/Unicode.hpp"

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
	const auto windowSize = GetWindow() ? GetWindow()->GetSize() : Size{1920, 1080};
	const float height = static_cast<float>(std::min<std::size_t>(7, m_Items.size())) * 32 + 8;
	const float width = std::min(std::max(160.0f, bounds.m_Width), windowSize.m_Width);
	m_PopupBounds = {std::clamp(bounds.m_X, 0.0f, std::max(0.0f, windowSize.m_Width - width)),
		bounds.Bottom() + height + 4 <= windowSize.m_Height ? bounds.Bottom() + 4 : std::max(0.0f, bounds.m_Y - height - 4), width, height};
	m_Highlight = std::max(0, m_Selected);
	m_TopIndex = std::max(0, m_Highlight - 6);
	ShowPopup([this](Canvas& canvas) { PaintPopup(canvas); }, [this](PointerEvent& event)
	{
		if (event.m_Button != PointerButton::Left) return false;
		if (m_PopupBounds.Contains(event.m_Position))
		{
			const int row = static_cast<int>((event.m_Position.m_Y - m_PopupBounds.m_Y - 4) / 32) + m_TopIndex;
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
		m_TopIndex = std::clamp(m_TopIndex, std::max(0, m_Highlight - 6), m_Highlight);
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
	for (int i = m_TopIndex; i < std::min(m_TopIndex + 7, static_cast<int>(m_Items.size())); ++i)
	{
		const float y = m_PopupBounds.m_Y + 4 + static_cast<float>(i - m_TopIndex) * 32;
		if (i == m_Highlight) canvas.FillRectangle({m_PopupBounds.m_X + 4, y, m_PopupBounds.m_Width - 8, 32}, theme.m_Hover, 4);
		canvas.DrawText(m_Items[static_cast<std::size_t>(i)], {m_PopupBounds.m_X + 12, y + 5}, theme.m_Text, GetFontSize());
	}
	canvas.PopClip();
}
}
