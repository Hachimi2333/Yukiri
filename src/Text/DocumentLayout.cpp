#include "DocumentLayout.hpp"
#include "Unicode.hpp"
#include <cmath>

namespace Yukiri::Detail
{
void DocumentLayout::Update(std::string_view text, float width, bool wrap, float lineHeight, std::uint64_t generation, const TextMeasure& measure, const TextCarets& carets)
{
	std::vector<std::string_view> paragraphs;
	std::size_t start = 0;
	for (;;)
	{
		const auto newline = text.find('\n', start);
		paragraphs.push_back(text.substr(start, newline == std::string_view::npos ? text.size() - start : newline - start));
		if (newline == std::string_view::npos) break;
		start = newline + 1;
	}
	const bool resize = (wrap && m_Width != width) || m_Wrap != wrap || m_Generation != generation;
	std::size_t first = 0, tail = 0;
	while (first < paragraphs.size() && first < m_Paragraphs.size() && paragraphs[first] == m_Paragraphs[first]->m_Text) ++first;
	while (tail < paragraphs.size() - first && tail < m_Paragraphs.size() - first &&
		paragraphs[paragraphs.size() - tail - 1] == m_Paragraphs[m_Paragraphs.size() - tail - 1]->m_Text) ++tail;
	if (!resize && first == paragraphs.size() && first == m_Paragraphs.size() && m_LineHeight == lineHeight) return;
	std::vector<std::unique_ptr<Paragraph>> updated;
	updated.reserve(paragraphs.size());
	for (std::size_t index = 0; index < paragraphs.size(); ++index)
	{
		std::unique_ptr<Paragraph> paragraph;
		if (index < first) paragraph = std::move(m_Paragraphs[index]);
		else if (index >= paragraphs.size() - tail) paragraph = std::move(m_Paragraphs[m_Paragraphs.size() - (paragraphs.size() - index)]);
		else { paragraph = std::make_unique<Paragraph>(); paragraph->m_Text = paragraphs[index]; }
		if (resize || paragraph->m_Lines.empty())
		{
			paragraph->m_Lines.clear();
			for (const auto& line : LayoutText(paragraph->m_Text, width, wrap, measure, carets))
			{
				const auto value = std::string_view(paragraph->m_Text).substr(line.m_Start, line.m_End - line.m_Start);
				auto positions = carets ? carets(value) : std::vector<std::pair<std::size_t, float>>{};
				if (positions.empty())
					for (const auto boundary : GraphemeBoundaries(value)) positions.emplace_back(boundary, measure(value.substr(0, boundary)).m_Width);
				paragraph->m_Lines.push_back({line.m_Start, line.m_End, line.m_Width, std::make_shared<const std::vector<std::pair<std::size_t, float>>>(std::move(positions))});
			}
		}
		updated.push_back(std::move(paragraph));
	}
	m_Paragraphs = std::move(updated);
	m_Width = width; m_Wrap = wrap; m_Generation = generation;
	m_LineHeight = std::max(1.0f, lineHeight);
	m_Lines.clear();
	m_Size = {};
	std::size_t offset = 0;
	for (const auto& paragraph : m_Paragraphs)
	{
		for (auto line : paragraph->m_Lines)
		{
			line.m_Start += offset;
			line.m_End += offset;
			m_Size.m_Width = std::max(m_Size.m_Width, line.m_Width);
			m_Lines.push_back(std::move(line));
		}
		offset += paragraph->m_Text.size() + 1;
	}
	m_Size.m_Height = static_cast<float>(m_Lines.size()) * m_LineHeight;
}
const std::vector<DocumentLayout::Line>& DocumentLayout::GetLines() const noexcept { return m_Lines; }
Size DocumentLayout::GetSize() const noexcept { return m_Size; }
float DocumentLayout::GetLineHeight() const noexcept { return m_LineHeight; }
std::size_t DocumentLayout::LineAt(std::size_t offset, bool upstream) const
{
	if (m_Lines.empty()) return 0;
	auto found = std::upper_bound(m_Lines.begin(), m_Lines.end(), offset, [](auto position, const auto& line) { return position < line.m_Start; });
	auto index = found == m_Lines.begin() ? std::size_t(0) : static_cast<std::size_t>(found - m_Lines.begin() - 1);
	if (upstream && index > 0 && m_Lines[index].m_Start == offset && m_Lines[index - 1].m_End == offset) --index;
	return index;
}
float DocumentLayout::CaretX(std::size_t index, std::size_t offset) const
{
	if (index >= m_Lines.size()) return 0;
	const auto& line = m_Lines[index];
	const auto relative = std::clamp(offset, line.m_Start, line.m_End) - line.m_Start;
	const auto& carets = *line.m_Carets;
	auto found = std::ranges::upper_bound(carets, relative, {}, &std::pair<std::size_t, float>::first);
	return found == carets.begin() ? 0.0f : std::prev(found)->second;
}
std::size_t DocumentLayout::Hit(std::size_t index, float x) const
{
	if (m_Lines.empty()) return 0;
	index = std::min(index, m_Lines.size() - 1);
	const auto& line = m_Lines[index];
	const auto& carets = *line.m_Carets;
	auto after = std::ranges::lower_bound(carets, x, {}, &std::pair<std::size_t, float>::second);
	if (after == carets.begin()) return line.m_Start;
	if (after == carets.end()) return line.m_End;
	const auto previous = std::prev(after);
	return line.m_Start + (x - previous->second < after->second - x ? previous->first : after->first);
}
std::pair<std::size_t, std::size_t> DocumentLayout::Visible(float top, float height) const
{
	const auto first = static_cast<std::size_t>(std::max(0.0f, std::floor(top / m_LineHeight)));
	const auto last = static_cast<std::size_t>(std::max(0.0f, std::ceil((top + height) / m_LineHeight)));
	return {std::min(first, m_Lines.size()), std::min(last, m_Lines.size())};
}
}
