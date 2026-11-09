#pragma once

#include "Unicode.hpp"
#include <algorithm>
#include <chrono>
#include <optional>

namespace Yukiri::Detail
{
class TextEditor
{
public:
	explicit TextEditor(bool singleLine = true) : m_SingleLine(singleLine) {}
	void SetText(std::string_view text);
	void Insert(std::string_view text, bool merge = false);
	void Replace(std::size_t start, std::size_t end, std::string_view text);
	void Select(std::size_t anchor, std::size_t caret);
	void SelectAll();
	void SelectWord(std::size_t offset);
	void SelectLine(std::size_t offset);
	void MoveLeft(bool select, bool word = false);
	void MoveRight(bool select, bool word = false);
	void MoveTo(std::size_t offset, bool select);
	void Backspace(bool word = false);
	void Delete(bool word = false);
	void Undo();
	void Redo();
	void BreakUndoGroup();
	[[nodiscard]] bool CanUndo() const noexcept;
	[[nodiscard]] bool CanRedo() const noexcept;
	[[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>> Find(std::string_view pattern, std::size_t from, bool matchCase, bool backwards = false) const;
	std::size_t ReplaceAll(std::string_view pattern, std::string_view replacement, bool matchCase);
	[[nodiscard]] std::size_t SelectionStart() const { return std::min(m_Caret, m_Anchor); }
	[[nodiscard]] std::size_t SelectionEnd() const { return std::max(m_Caret, m_Anchor); }
	[[nodiscard]] std::string GetSelection() const { return m_Text.substr(SelectionStart(), SelectionEnd() - SelectionStart()); }
	std::string m_Text;
	std::size_t m_Caret = 0;
	std::size_t m_Anchor = 0;
	std::uint64_t m_Revision = 0;
private:
	enum class EditKind { Other, Insert, Backspace, Delete };
	struct Change
	{
		std::size_t m_Start;
		std::string m_Before;
		std::string m_After;
		std::size_t m_PreviousCaret;
		std::size_t m_PreviousAnchor;
		std::size_t m_Caret;
		std::size_t m_Anchor;
	};
	void Apply(std::size_t start, std::size_t end, std::string text, EditKind kind);
	bool m_SingleLine;
	EditKind m_LastKind = EditKind::Other;
	std::chrono::steady_clock::time_point m_LastEdit{};
	std::vector<Change> m_Undo;
	std::vector<Change> m_Redo;
};
}
