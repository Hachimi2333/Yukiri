#pragma once

#include "TextEditor.hpp"

namespace Yukiri::Detail
{
struct TextDisplay
{
	std::string m_Text;
	std::size_t m_Caret = 0;
	std::size_t m_SelectionStart = 0;
	std::size_t m_SelectionEnd = 0;
	std::size_t m_CompositionStart = 0;
	std::size_t m_CompositionEnd = 0;
	bool m_Composing = false;
};

class TextComposition
{
public:
	void Update(std::string_view text, int start, int length, bool singleLine = true)
	{
		m_Text = NormalizeText(text, singleLine);
		const auto points = DecodeUtf8(m_Text);
		const auto caret = start < 0 ? points.size() : std::min(static_cast<std::size_t>(start), points.size());
		const auto selected = std::min(static_cast<std::size_t>(std::max(0, length)), points.size() - caret);
		auto offset = [&](std::size_t index) { return index < points.size() ? points[index].m_Start : m_Text.size(); };
		m_Caret = offset(caret);
		m_SelectionEnd = offset(caret + selected);
	}
	void Clear() noexcept { m_Text.clear(); m_Caret = m_SelectionEnd = 0; }
	[[nodiscard]] bool IsActive() const noexcept { return !m_Text.empty(); }
	[[nodiscard]] TextDisplay BuildDisplay(const TextEditor& editor) const
	{
		if (!IsActive()) return {editor.m_Text, editor.m_Caret, editor.SelectionStart(), editor.SelectionEnd()};
		const auto start = editor.SelectionStart();
		return {editor.m_Text.substr(0, start) + m_Text + editor.m_Text.substr(editor.SelectionEnd()),
			start + m_Caret, start + m_Caret, start + m_SelectionEnd, start, start + m_Text.size(), true};
	}

private:
	std::string m_Text;
	std::size_t m_Caret = 0;
	std::size_t m_SelectionEnd = 0;
};
}
