#include "TextEditor.hpp"
#include <numeric>

namespace Yukiri::Detail
{
void TextEditor::SetText(std::string_view text)
{
	m_Text = NormalizeText(text, m_SingleLine);
	++m_Revision;
	m_Caret = m_Anchor = m_Text.size();
	m_Undo.clear();
	m_Redo.clear();
	BreakUndoGroup();
}
void TextEditor::BreakUndoGroup() { m_LastKind = EditKind::Other; }
bool TextEditor::CanUndo() const noexcept { return !m_Undo.empty(); }
bool TextEditor::CanRedo() const noexcept { return !m_Redo.empty(); }
void TextEditor::Apply(std::size_t start, std::size_t end, std::string text, EditKind kind)
{
	if (start == end && text.empty()) return;
	Change change{start, m_Text.substr(start, end - start), std::move(text), m_Caret, m_Anchor, 0, 0};
	if (change.m_Before == change.m_After) { MoveTo(start + change.m_After.size(), false); return; }
	m_Text.replace(start, end - start, change.m_After);
	++m_Revision;
	m_Caret = m_Anchor = start + change.m_After.size();
	change.m_Caret = m_Caret;
	change.m_Anchor = m_Anchor;
	const auto now = std::chrono::steady_clock::now();
	bool merged = false;
	if (!m_Undo.empty() && kind != EditKind::Other && kind == m_LastKind && now - m_LastEdit < std::chrono::milliseconds(750))
	{
		auto& previous = m_Undo.back();
		if (kind == EditKind::Insert && previous.m_Start + previous.m_After.size() == start && change.m_Before.empty())
		{
			previous.m_After += change.m_After;
			merged = true;
		}
		else if (kind == EditKind::Backspace && start + change.m_Before.size() == previous.m_Start)
		{
			previous.m_Start = start;
			previous.m_Before.insert(0, change.m_Before);
			merged = true;
		}
		else if (kind == EditKind::Delete && previous.m_Start == start) { previous.m_Before += change.m_Before; merged = true; }
		if (merged) { previous.m_Caret = m_Caret; previous.m_Anchor = m_Anchor; }
	}
	if (!merged) m_Undo.push_back(std::move(change));
	m_LastKind = kind;
	m_LastEdit = now;
	m_Redo.clear();
	auto bytes = std::accumulate(m_Undo.begin(), m_Undo.end(), std::size_t(0), [](auto sum, const auto& edit) { return sum + edit.m_Before.size() + edit.m_After.size(); });
	while (m_Undo.size() > 1 && (m_Undo.size() > 128 || bytes > 16 * 1024 * 1024))
	{
		bytes -= m_Undo.front().m_Before.size() + m_Undo.front().m_After.size();
		m_Undo.erase(m_Undo.begin());
	}
}
void TextEditor::Insert(std::string_view text, bool merge)
{
	auto sanitized = NormalizeText(text, m_SingleLine);
	const bool typing = merge && m_Caret == m_Anchor && sanitized.find('\n') == std::string::npos;
	Apply(SelectionStart(), SelectionEnd(), std::move(sanitized), typing ? EditKind::Insert : EditKind::Other);
}
void TextEditor::Replace(std::size_t start, std::size_t end, std::string_view text)
{
	const auto oldCaret = m_Caret, oldAnchor = m_Anchor;
	Select(start, end);
	const auto first = SelectionStart(), last = SelectionEnd();
	m_Caret = oldCaret; m_Anchor = oldAnchor;
	Apply(first, last, NormalizeText(text, m_SingleLine), EditKind::Other);
}
void TextEditor::Select(std::size_t anchor, std::size_t caret)
{
	BreakUndoGroup();
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
void TextEditor::SelectAll() { Select(0, m_Text.size()); }
void TextEditor::SelectWord(std::size_t offset)
{
	const auto breaks = WordBoundaries(m_Text);
	const auto right = std::ranges::upper_bound(breaks, std::min(offset, m_Text.empty() ? 0 : m_Text.size() - 1));
	const auto left = right == breaks.begin() ? 0 : *std::prev(right);
	Select(left, right == breaks.end() ? m_Text.size() : *right);
}
void TextEditor::SelectLine(std::size_t offset)
{
	offset = std::min(offset, m_Text.size());
	const auto start = offset ? m_Text.rfind('\n', offset - 1) : std::string::npos;
	const auto end = m_Text.find('\n', offset);
	Select(start == std::string::npos ? 0 : start + 1, end == std::string::npos ? m_Text.size() : end + 1);
}
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
	const bool selection = m_Caret != m_Anchor;
	const auto start = selection ? SelectionStart() : PreviousBoundary(word ? WordBoundaries(m_Text) : GraphemeBoundaries(m_Text), m_Caret);
	Apply(start, SelectionEnd(), {}, selection || word ? EditKind::Other : EditKind::Backspace);
}
void TextEditor::Delete(bool word)
{
	if (m_Caret == m_Anchor && m_Caret == m_Text.size()) return;
	const bool selection = m_Caret != m_Anchor;
	const auto end = selection ? SelectionEnd() : NextBoundary(word ? WordBoundaries(m_Text) : GraphemeBoundaries(m_Text), m_Caret);
	Apply(SelectionStart(), end, {}, selection || word ? EditKind::Other : EditKind::Delete);
}
void TextEditor::Undo()
{
	if (m_Undo.empty()) return;
	BreakUndoGroup();
	auto change = std::move(m_Undo.back()); m_Undo.pop_back();
	m_Text.replace(change.m_Start, change.m_After.size(), change.m_Before);
	++m_Revision;
	m_Caret = change.m_PreviousCaret; m_Anchor = change.m_PreviousAnchor;
	m_Redo.push_back(std::move(change));
}
void TextEditor::Redo()
{
	if (m_Redo.empty()) return;
	BreakUndoGroup();
	auto change = std::move(m_Redo.back()); m_Redo.pop_back();
	m_Text.replace(change.m_Start, change.m_Before.size(), change.m_After);
	++m_Revision;
	m_Caret = change.m_Caret; m_Anchor = change.m_Anchor;
	m_Undo.push_back(std::move(change));
}
}
