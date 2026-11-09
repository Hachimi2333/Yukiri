#include <Yukiri/Controls/Label.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/Unicode.hpp"
#include "Text/TextLayout.hpp"
#include <Yukiri/Platform/Window.hpp>
#include <cmath>

namespace Yukiri
{
Label::Label(std::string text) : m_Text(Detail::SanitizeUtf8(text)) {}
void Label::SetText(std::string_view text)
{
	auto sanitized = Detail::SanitizeUtf8(text);
	if (m_Text == sanitized) return;
	m_Text = std::move(sanitized);
	InvalidateLayout();
}
const std::string& Label::GetText() const noexcept { return m_Text; }
void Label::SetWrap(bool wrap) { if (m_Wrap == wrap) return; m_Wrap = wrap; InvalidateLayout(); }
void Label::SetEllipsis(bool ellipsis) { m_Ellipsis = ellipsis; InvalidateLayout(); }
void Label::SetTextAlignment(Alignment alignment) { m_TextAlignment = alignment; InvalidatePaint(); }
std::vector<std::string> Label::Lines(float width) const
{
	std::vector<std::string> lines;
	const auto layout = Detail::LayoutText(m_Text, width, m_Wrap, [this](std::string_view text) { return MeasureText(text); },
		GetWindow() ? Detail::TextCarets([this](std::string_view text) { return GetWindow()->GetTextCarets(text, GetFontSize(), GetFont()); }) : Detail::TextCarets{});
	for (const auto& line : layout)
	{
		lines.emplace_back(std::string_view(m_Text).substr(line.m_Start, line.m_End - line.m_Start));
	}
	return lines;
}
Size Label::OnMeasure(Size available)
{
	Size size;
	for (const auto& line : Lines(available.m_Width))
	{
		const auto measured = MeasureText(line);
		size.m_Width = std::max(size.m_Width, measured.m_Width);
		size.m_Height += measured.m_Height;
	}
	if (m_Ellipsis && !m_Wrap) size.m_Width = std::min(size.m_Width, available.m_Width);
	return size;
}
void Label::OnPaint(Canvas& canvas)
{
	Widget::OnPaint(canvas);
	const auto padding = GetPadding();
	const auto width = std::max(0.0f, GetActualSize().m_Width - padding.Horizontal());
	float y = padding.m_Top;
	canvas.PushClip({padding.m_Left, padding.m_Top, width, std::max(0.0f, GetActualSize().m_Height - padding.Vertical())});
	for (const auto& original : Lines(width))
	{
		const auto line = m_Ellipsis && !m_Wrap ? Detail::FitText(original, width, [this](std::string_view text) { return MeasureText(text); }) : original;
		const auto measured = canvas.MeasureText(line, GetFontSize());
		const float extra = std::max(0.0f, width - measured.m_Width);
		const float x = padding.m_Left + (m_TextAlignment == Alignment::Center ? extra * 0.5f : m_TextAlignment == Alignment::End ? extra : 0);
		canvas.DrawText(line, {x, y}, GetForeground(), GetFontSize());
		y += measured.m_Height;
	}
	canvas.PopClip();
}
}
