#include <Yukiri/Controls/Label.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/Unicode.hpp"
#include <cmath>

namespace Yukiri
{
Label::Label(std::string text) : m_Text(Detail::SanitizeUtf8(text)) {}
void Label::SetText(std::string_view text) { m_Text = Detail::SanitizeUtf8(text); InvalidateLayout(); }
const std::string& Label::GetText() const noexcept { return m_Text; }
void Label::SetWrap(bool wrap) { m_Wrap = wrap; InvalidateLayout(); }
std::vector<std::string> Label::Lines(float width) const
{
	std::vector<std::string> lines;
	std::size_t paragraph = 0;
	for (;;)
	{
		const auto end = m_Text.find('\n', paragraph);
		const auto text = std::string_view(m_Text).substr(paragraph, end == std::string::npos ? end : end - paragraph);
		if (!m_Wrap || !std::isfinite(width) || width <= 0) lines.emplace_back(text);
		else
		{
			const auto boundaries = Detail::LineBoundaries(text);
			std::size_t start = 0;
			std::size_t accepted = 0;
			for (std::size_t i = 1; i < boundaries.size(); ++i)
			{
				const auto next = boundaries[i];
				if (accepted > start && MeasureText(text.substr(start, next - start)).m_Width > width)
				{
					lines.emplace_back(text.substr(start, accepted - start));
					start = accepted;
				}
				accepted = next;
			}
			lines.emplace_back(text.substr(start));
		}
		if (end == std::string::npos) break;
		paragraph = end + 1;
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
	return size;
}
void Label::OnPaint(Canvas& canvas)
{
	Widget::OnPaint(canvas);
	float y = 0;
	for (const auto& line : Lines(GetActualSize().m_Width))
	{
		canvas.DrawText(line, {0, y}, GetForeground(), GetFontSize());
		y += canvas.MeasureText(line, GetFontSize()).m_Height;
	}
}
}
