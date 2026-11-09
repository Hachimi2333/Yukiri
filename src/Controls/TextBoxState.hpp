#pragma once

#include <Yukiri/Controls/TextBox.hpp>
#include "Text/TextVisual.hpp"
#include "Text/DocumentLayout.hpp"

namespace Yukiri
{
struct TextBox::Impl
{
	explicit Impl(bool multiline) : m_Editor(!multiline), m_Multiline(multiline) {}
	std::size_t ToCommittedOffset(std::size_t visual) const
	{
		const auto offset = m_Visual.ToSource(visual);
		if (!m_Composition.IsActive()) return std::min(offset, m_Editor.m_Text.size());
		const auto start = m_Editor.SelectionStart();
		const auto end = m_Visual.ToSource(m_Visual.m_Display.m_CompositionEnd);
		return std::min(m_Editor.m_Text.size(), offset <= start ? offset : offset < end ? start : offset - end + m_Editor.SelectionEnd());
	}
	std::size_t ToVisualOffset(std::size_t committed, bool endPoint = false) const
	{
		committed = std::min(committed, m_Editor.m_Text.size());
		if (m_Composition.IsActive())
		{
			const auto start = m_Editor.SelectionStart();
			const auto end = m_Visual.ToSource(m_Visual.m_Display.m_CompositionEnd);
			if (committed >= m_Editor.SelectionEnd()) committed = committed - m_Editor.SelectionEnd() + end;
			else if (committed > start) committed = endPoint ? end : start;
		}
		return m_Visual.ToVisual(committed);
	}
	Detail::TextEditor m_Editor;
	Detail::TextComposition m_Composition;
	Detail::TextVisual m_Visual;
	Detail::DocumentLayout m_Document;
	std::string m_Placeholder;
	bool m_Multiline = false;
	bool m_Wrap = true;
	bool m_Password = false;
	bool m_ReadOnly = false;
	bool m_Dragging = false;
	bool m_DirtyVisual = true;
	bool m_EnsureCaret = true;
	bool m_Upstream = false;
	bool m_DocumentDirty = true;
	bool m_PreparedWrap = true;
	Size m_PreparedSize{-1, -1};
	int m_DragAxis = 0;
	int m_SelectionMode = 1;
	std::pair<std::size_t, std::size_t> m_DragSelection;
	std::pair<std::size_t, std::size_t> m_LastSelection;
	std::optional<float> m_PreferredX;
	Point m_DragPointer;
	Point m_DragStart;
	Point m_DragOffset;
	Point m_Scroll;
	Point m_Target;
	Detail::Transition m_ScrollX;
	Detail::Transition m_ScrollY;
	Rect m_Viewport;
	Font m_LayoutFont;
	float m_LayoutFontSize = -1;
	float m_LayoutScale = -1;
	std::uint64_t m_LayoutGeneration = 0;
	std::uint64_t m_LayoutSourceGeneration = 0;
	std::uint64_t m_VisualRevision = 0;
	std::chrono::steady_clock::time_point m_LastInteraction = std::chrono::steady_clock::now();
};
}
