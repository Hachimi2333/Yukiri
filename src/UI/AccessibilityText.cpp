#include "AccessibilityTree.hpp"
#include "Controls/TextBoxState.hpp"

namespace Yukiri::Detail
{
std::vector<std::size_t> AccessibilityTree::TextBoundaries(const AccessibleNode& node, int unit) const
{
	auto* text = TextControl(node);
	if (!text) return {0};
	const auto& value = text->GetText();
	if (unit == 0) return GraphemeBoundaries(value);
	if (unit == 1) return WordBoundaries(value);
	if (unit == 3)
	{
		std::vector<std::size_t> result{0};
		for (std::size_t offset = 0; offset < value.size(); ++offset) if (value[offset] == '\n') result.push_back(offset + 1);
		if (result.back() != value.size()) result.push_back(value.size());
		return result;
	}
	if (unit == 2 || unit == 4)
	{
		text->PrepareLayout();
		std::vector<std::size_t> result{0};
		const auto& lines = text->m_Impl->m_Document.GetLines();
		const auto stride = unit == 4 ? std::max(std::size_t(1), static_cast<std::size_t>(text->m_Impl->m_Viewport.m_Height / text->m_Impl->m_Document.GetLineHeight())) : 1;
		for (std::size_t index = stride; index < lines.size(); index += stride)
		{
			const auto offset = text->m_Impl->ToCommittedOffset(lines[index].m_Start);
			if (offset > result.back()) result.push_back(offset);
		}
		if (result.back() != value.size()) result.push_back(value.size());
		return result;
	}
	return value.empty() ? std::vector<std::size_t>{0} : std::vector<std::size_t>{0, value.size()};
}
std::pair<std::size_t, std::size_t> AccessibilityTree::VisibleText(const AccessibleNode& node) const
{
	auto* text = TextControl(node);
	if (!text) return {};
	text->PrepareLayout();
	const auto viewport = text->m_Impl->m_Viewport, bounds = text->GetBounds();
	const Rect global{bounds.m_X + viewport.m_X, bounds.m_Y + viewport.m_Y, viewport.m_Width, viewport.m_Height};
	const auto clipped = Clip(*text, global);
	if (clipped.Empty()) return {};
	const auto visible = text->m_Impl->m_Document.Visible(text->GetOffset().m_Y + clipped.m_Y - global.m_Y, clipped.m_Height);
	const auto& lines = text->m_Impl->m_Document.GetLines();
	if (visible.first >= visible.second || visible.first >= lines.size()) return {};
	const float left = text->GetOffset().m_X + clipped.m_X - global.m_X;
	const auto& firstLine = lines[visible.first];
	const auto& lastLine = lines[visible.second - 1];
	const auto& firstCarets = *firstLine.m_Carets;
	const auto& lastCarets = *lastLine.m_Carets;
	auto first = std::ranges::upper_bound(firstCarets, left, {}, &std::pair<std::size_t, float>::second);
	if (first != firstCarets.begin()) --first;
	auto last = std::ranges::lower_bound(lastCarets, left + clipped.m_Width, {}, &std::pair<std::size_t, float>::second);
	const auto start = first == firstCarets.end() ? firstLine.m_End : firstLine.m_Start + first->first;
	const auto end = last == lastCarets.end() ? lastLine.m_End : lastLine.m_Start + last->first;
	return {text->m_Impl->ToCommittedOffset(start), text->m_Impl->ToCommittedOffset(end)};
}
std::vector<Rect> AccessibilityTree::TextRectangles(const AccessibleNode& node, std::size_t start, std::size_t end) const
{
	std::vector<Rect> result;
	if (auto* text = TextControl(node)) for (const auto rect : text->GetTextBounds(start, end)) { const auto clipped = Clip(*text, rect); if (!clipped.Empty()) result.push_back(clipped); }
	return result;
}
Result<void> AccessibilityTree::ScrollText(const AccessibleNode& node, std::size_t start, std::size_t end, bool top)
{
	auto* text = TextControl(node);
	if (!text || !Describe(node).m_Enabled) return std::unexpected(Error{ErrorCode::InvalidArgument, "The text range cannot be scrolled."});
	Dispatch([&]
	{
		text->PrepareLayout();
		const auto offset = text->m_Impl->ToVisualOffset(top ? start : end, !top);
		const auto line = text->m_Impl->m_Document.LineAt(offset);
		const float y = static_cast<float>(line) * text->m_Impl->m_Document.GetLineHeight();
		const float x = text->m_Impl->m_Document.CaretX(line, offset);
		auto position = text->GetOffset();
		position.m_Y = top ? y : y + text->m_Impl->m_Document.GetLineHeight() - text->m_Impl->m_Viewport.m_Height;
		if (x < position.m_X) position.m_X = x;
		else if (x > position.m_X + text->m_Impl->m_Viewport.m_Width - 2) position.m_X = x - text->m_Impl->m_Viewport.m_Width + 2;
		text->SetOffset(position);
	});
	return {};
}
}
