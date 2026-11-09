#include "ItemViewState.hpp"
#include <Yukiri/Render/Canvas.hpp>
#include "Text/TextLayout.hpp"

namespace Yukiri
{
Rect ListView::VerticalThumb() const
{
	const float height = static_cast<float>(GetRowCount()) * GetRowHeight();
	const auto viewport = m_Impl->m_Viewport;
	if (height <= viewport.m_Height || viewport.m_Height <= 0) return {};
	const float thumb = std::min(viewport.m_Height, std::max(24.0f, viewport.m_Height * viewport.m_Height / height));
	return {GetActualSize().m_Width - 9, viewport.m_Y + (viewport.m_Height - thumb) * m_Impl->m_Offset.m_Y / std::max(1.0f, height - viewport.m_Height), 5, thumb};
}
Rect ListView::HorizontalThumb() const
{
	const auto viewport = m_Impl->m_Viewport;
	if (ContentWidth() <= viewport.m_Width || viewport.m_Width <= 0) return {};
	const float thumb = std::min(viewport.m_Width, std::max(24.0f, viewport.m_Width * viewport.m_Width / ContentWidth()));
	return {viewport.m_X + (viewport.m_Width - thumb) * m_Impl->m_Offset.m_X / std::max(1.0f, ContentWidth() - viewport.m_Width), GetActualSize().m_Height - 9, thumb, 5};
}
void ListView::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const auto size = GetActualSize();
	canvas.FillRectangle(GetLocalBounds(), GetStyle().m_Background.value_or(theme.m_Surface), GetRadius());
	canvas.StrokeRectangle(GetLocalBounds(), GetStyle().m_Border.value_or(theme.m_Border), 1, GetRadius());
	if (!m_Impl->m_Model) { PaintFocus(canvas); return; }
	if (HeaderHeight() > 0)
	{
		canvas.PushClip({2, 1, std::max(0.0f, size.m_Width - 4), HeaderHeight()});
		canvas.FillRectangle({1, 1, size.m_Width - 2, HeaderHeight()}, theme.m_Hover);
		const auto sort = m_Impl->m_Model->GetSort();
		for (std::size_t column = 0; column < GetColumnCount(); ++column)
		{
			const float left = 2 + ColumnLeft(column) - m_Impl->m_Offset.m_X, width = GetColumnWidthImpl(column);
			if (left + width < 0 || left > size.m_Width) continue;
			canvas.PushClip({left + 8, 2, std::max(0.0f, width - 16), HeaderHeight() - 2});
			const auto caption = Detail::FitText(m_Impl->m_Model->GetHeader(column), std::max(0.0f, width - 32), [&](auto text) { return canvas.MeasureText(text, GetFontSize()); });
			canvas.DrawText(caption, {left + 8, (HeaderHeight() - GetFontSize() * 1.5f) / 2}, GetForeground(), GetFontSize());
			if (sort && sort->first == column)
			{
				const float x = left + width - 15, y = HeaderHeight() / 2;
				const float direction = sort->second ? -1.0f : 1.0f;
				canvas.DrawLine({x - 4, y - direction * 2}, {x, y + direction * 2}, theme.m_MutedText);
				canvas.DrawLine({x, y + direction * 2}, {x + 4, y - direction * 2}, theme.m_MutedText);
			}
			canvas.PopClip();
			canvas.DrawLine({left + width, 4}, {left + width, HeaderHeight() - 4}, theme.m_Border);
		}
		canvas.PopClip();
	}
	canvas.PushClip(m_Impl->m_Viewport);
	const auto visible = GetVisibleRange();
	for (std::size_t row = visible.first; row < visible.second; ++row)
	{
		const auto& entry = m_Impl->m_Rows[row];
		const auto rowRect = CellBounds(row, 0);
		const bool selected = m_Impl->m_Selection->IsSelected(entry.m_Id);
		if (selected) canvas.FillRectangle({m_Impl->m_Viewport.m_X, rowRect.m_Y, m_Impl->m_Viewport.m_Width, rowRect.m_Height}, theme.m_Selection, 3);
		else if (m_Impl->m_Kind == ViewKind::Table && row % 2) canvas.FillRectangle({m_Impl->m_Viewport.m_X, rowRect.m_Y, m_Impl->m_Viewport.m_Width, rowRect.m_Height}, theme.m_Hover.WithAlpha(0.35f));
		for (std::size_t column = 0; column < GetColumnCount(); ++column)
		{
			const auto rect = CellBounds(row, column);
			if (rect.Intersect(m_Impl->m_Viewport).Empty()) continue;
			const auto cell = m_Impl->m_Model->GetCell(entry.m_Id, column);
			const auto color = cell.m_Enabled && IsEnabled() ? GetForeground() : theme.m_MutedText;
			float left = rect.m_X + 8;
			canvas.PushClip(rect);
			if (m_Impl->m_Kind == ViewKind::Tree)
			{
				left += static_cast<float>(entry.m_Depth) * 18;
				if (entry.m_HasChildren)
				{
					const float y = rect.m_Y + rect.m_Height / 2;
					if (IsExpanded(entry.m_Id)) { canvas.DrawLine({left, y - 2}, {left + 4, y + 2}, color); canvas.DrawLine({left + 4, y + 2}, {left + 8, y - 2}, color); }
					else { canvas.DrawLine({left + 2, y - 4}, {left + 6, y}, color); canvas.DrawLine({left + 6, y}, {left + 2, y + 4}, color); }
				}
				left += 18;
			}
			if (cell.m_Checked)
			{
				const Rect box{left, rect.m_Y + (rect.m_Height - 16) / 2, 16, 16};
				canvas.FillRectangle(box, *cell.m_Checked ? theme.m_Accent : theme.m_Surface, 3);
				canvas.StrokeRectangle(box, *cell.m_Checked ? theme.m_Accent : theme.m_Border, 1, 3);
				if (*cell.m_Checked) { canvas.DrawLine({left + 3, box.m_Y + 8}, {left + 7, box.m_Y + 12}, theme.m_OnAccent, 1.5f); canvas.DrawLine({left + 7, box.m_Y + 12}, {left + 13, box.m_Y + 4}, theme.m_OnAccent, 1.5f); }
				left += 24;
			}
			if (cell.m_Icon) { canvas.DrawImage(cell.m_Icon, {left, rect.m_Y + (rect.m_Height - 18) / 2, 18, 18}, cell.m_Enabled ? 1.0f : 0.5f); left += 26; }
			if (m_Impl->m_EditingItem != entry.m_Id || m_Impl->m_EditingColumn != column)
			{
				const auto text = Detail::FitText(cell.m_Text, std::max(0.0f, rect.Right() - left - 8), [&](auto value) { return canvas.MeasureText(value, GetFontSize()); });
				canvas.DrawText(text, {left, rect.m_Y + (rect.m_Height - GetFontSize() * 1.5f) / 2}, color, GetFontSize());
			}
			canvas.PopClip();
			if (m_Impl->m_Kind == ViewKind::Table) canvas.DrawLine({rect.Right(), rect.m_Y}, {rect.Right(), rect.Bottom()}, theme.m_Border.WithAlpha(0.5f));
		}
	}
	PaintFocus(canvas);
	canvas.PopClip();
	canvas.FillRectangle(VerticalThumb(), theme.m_MutedText.WithAlpha(0.55f), 3);
	canvas.FillRectangle(HorizontalThumb(), theme.m_MutedText.WithAlpha(0.55f), 3);
}
}
