#include "TextEditor.hpp"

namespace Yukiri::Detail
{
void TextEditor::SetText(std::string_view text)
{
	m_Text = SanitizeUtf8(text, true);
	m_Caret = m_Anchor = m_Text.size();
	m_Undo.clear();
	m_Redo.clear();
}
void TextEditor::SaveUndo()
{
	m_Undo.push_back({m_Text, m_Caret, m_Anchor});
	if (m_Undo.size() > 128) m_Undo.erase(m_Undo.begin());
	m_Redo.clear();
}
void TextEditor::EraseSelection()
{
	const auto start = SelectionStart();
	m_Text.erase(start, SelectionEnd() - start);
	m_Caret = m_Anchor = start;
}
void TextEditor::Insert(std::string_view text)
{
	const auto sanitized = SanitizeUtf8(text, true);
	if (sanitized.empty() && m_Caret == m_Anchor) return;
	SaveUndo();
	EraseSelection();
	m_Text.insert(m_Caret, sanitized);
	m_Caret += sanitized.size();
	m_Anchor = m_Caret;
}
void TextEditor::Select(std::size_t anchor, std::size_t caret)
{
	const auto boundaries = GraphemeBoundaries(m_Text);
	auto floor = [&](std::size_t value)
	{
		value = std::min(value, m_Text.size());
		auto next = std::ranges::upper_bound(boundaries, value);
		return next == boundaries.begin() ? std::size_t(0) : *std::prev(next);
	};
	m_Anchor = floor(anchor);
	m_Caret = floor(caret);
}
void TextEditor::SelectAll() { m_Anchor = 0; m_Caret = m_Text.size(); }
void TextEditor::MoveTo(std::size_t offset, bool select) { Select(select ? m_Anchor : offset, offset); }
void TextEditor::MoveLeft(bool select, bool word)
{
	const auto offset = !select && m_Caret != m_Anchor ? SelectionStart() :
		PreviousBoundary(word ? WordBoundaries(m_Text) : GraphemeBoundaries(m_Text), m_Caret);
	MoveTo(offset, select);
}
void TextEditor::MoveRight(bool select, bool word)
{
	const auto offset = !select && m_Caret != m_Anchor ? SelectionEnd() :
		NextBoundary(word ? WordBoundaries(m_Text) : GraphemeBoundaries(m_Text), m_Caret);
	MoveTo(offset, select);
}
void TextEditor::Backspace(bool word)
{
	if (m_Caret == m_Anchor && m_Caret == 0) return;
	SaveUndo();
	if (m_Caret == m_Anchor)
	{
		m_Anchor = PreviousBoundary(word ? WordBoundaries(m_Text) : GraphemeBoundaries(m_Text), m_Caret);
	}
	EraseSelection();
}
void TextEditor::Delete(bool word)
{
	if (m_Caret == m_Anchor && m_Caret == m_Text.size()) return;
	SaveUndo();
	if (m_Caret == m_Anchor)
	{
		m_Anchor = NextBoundary(word ? WordBoundaries(m_Text) : GraphemeBoundaries(m_Text), m_Caret);
	}
	EraseSelection();
}
void TextEditor::Undo()
{
	if (m_Undo.empty()) return;
	m_Redo.push_back({m_Text, m_Caret, m_Anchor});
	auto snapshot = std::move(m_Undo.back());
	m_Undo.pop_back();
	m_Text = std::move(snapshot.m_Text);
	m_Caret = snapshot.m_Caret;
	m_Anchor = snapshot.m_Anchor;
}
void TextEditor::Redo()
{
	if (m_Redo.empty()) return;
	m_Undo.push_back({m_Text, m_Caret, m_Anchor});
	auto snapshot = std::move(m_Redo.back());
	m_Redo.pop_back();
	m_Text = std::move(snapshot.m_Text);
	m_Caret = snapshot.m_Caret;
	m_Anchor = snapshot.m_Anchor;
}
}
