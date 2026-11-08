#pragma once

#include "Unicode.hpp"
#include <algorithm>

namespace Yukiri::Detail
{
class TextEditor
{
public:
	void SetText(std::string_view text);
	void Insert(std::string_view text);
	void Select(std::size_t anchor, std::size_t caret);
	void SelectAll();
	void MoveLeft(bool select, bool word = false);
	void MoveRight(bool select, bool word = false);
	void MoveTo(std::size_t offset, bool select);
	void Backspace(bool word = false);
	void Delete(bool word = false);
	void Undo();
	void Redo();
	[[nodiscard]] std::size_t SelectionStart() const { return std::min(m_Caret, m_Anchor); }
	[[nodiscard]] std::size_t SelectionEnd() const { return std::max(m_Caret, m_Anchor); }
	[[nodiscard]] std::string GetSelection() const { return m_Text.substr(SelectionStart(), SelectionEnd() - SelectionStart()); }
	std::string m_Text;
	std::size_t m_Caret = 0;
	std::size_t m_Anchor = 0;

private:
	struct Snapshot { std::string m_Text; std::size_t m_Caret; std::size_t m_Anchor; };
	void SaveUndo();
	void EraseSelection();
	std::vector<Snapshot> m_Undo;
	std::vector<Snapshot> m_Redo;
};
}
