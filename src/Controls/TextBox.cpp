#include <Yukiri/Controls/TextBox.hpp>
#include <Yukiri/Platform/Window.hpp>
#include "TextBoxState.hpp"
#include <cmath>

namespace Yukiri
{
TextBox::TextBox(std::string text) : TextBox(std::move(text), false) {}
TextBox::TextBox(std::string text, bool multiline) : m_Impl(std::make_unique<Impl>(multiline))
{
	m_Impl->m_Editor.SetText(text);
	SetFocusable(true);
	SetCursor(Cursor::Text);
}
TextBox::~TextBox() { if (HasFocus() && GetWindow()) GetWindow()->StopTextInput(); }
const std::string& TextBox::GetText() const noexcept { return m_Impl->m_Editor.m_Text; }
bool TextBox::IsReadOnly() const noexcept { return m_Impl->m_ReadOnly; }
bool TextBox::IsPassword() const noexcept { return m_Impl->m_Password; }
std::string TextBox::GetAccessibleText() const { return IsPassword() ? std::string{} : GetText(); }
bool TextBox::CanUndo() const noexcept { return !IsReadOnly() && m_Impl->m_Editor.CanUndo(); }
bool TextBox::CanRedo() const noexcept { return !IsReadOnly() && m_Impl->m_Editor.CanRedo(); }
std::pair<std::size_t, std::size_t> TextBox::GetSelection() const noexcept { return {m_Impl->m_Editor.m_Anchor, m_Impl->m_Editor.m_Caret}; }
void TextBox::SetText(std::string_view text)
{
	Dispatch([&]
	{
		if (IsComposing()) CancelComposition();
		const auto normalized = Detail::NormalizeText(text, !m_Impl->m_Multiline);
		if (normalized == GetText()) return;
		const auto previous = m_Impl->m_Editor.m_Revision;
		m_Impl->m_Editor.SetText(normalized);
		EmitEdit(previous);
	});
}
void TextBox::SetPlaceholder(std::string_view placeholder) { m_Impl->m_Placeholder = Detail::NormalizeText(placeholder, !m_Impl->m_Multiline); InvalidatePaint(); }
void TextBox::SetPassword(bool password)
{
	if (m_Impl->m_Multiline || IsPassword() == password) return;
	Dispatch([&] { CancelComposition(); m_Impl->m_Password = password; m_Impl->m_DirtyVisual = true; RefreshCaret(); m_EditStateChanged.Emit(); });
}
void TextBox::SetReadOnly(bool readOnly)
{
	if (IsReadOnly() == readOnly) return;
	Dispatch([&]
	{
		m_Impl->m_ReadOnly = readOnly;
		if (readOnly)
		{
			CancelComposition();
			if (HasFocus() && GetWindow()) GetWindow()->StopTextInput();
		}
		else if (HasFocus() && GetWindow()) GetWindow()->StartTextInput(GetCaretBounds());
		InvalidatePaint();
		m_EditStateChanged.Emit();
	});
}
void TextBox::SetTextWrap(bool wrap)
{
	if (m_Impl->m_Wrap == wrap) return;
	m_Impl->m_Wrap = wrap;
	m_Impl->m_EnsureCaret = true;
	InvalidateLayout();
}
bool TextBox::GetTextWrap() const noexcept { return m_Impl->m_Wrap; }
bool TextBox::AcceptsTextInput() const noexcept { return !IsReadOnly(); }
bool TextBox::AcceptsReturn() const noexcept { return m_Impl->m_Multiline; }
bool TextBox::IsComposing() const noexcept { return m_Impl->m_Composition.IsActive(); }
void TextBox::RefreshCaret()
{
	m_Impl->m_LastInteraction = std::chrono::steady_clock::now();
	m_Impl->m_EnsureCaret = true;
	InvalidatePaint();
	const auto selection = GetSelection();
	if (selection != m_Impl->m_LastSelection) { m_Impl->m_LastSelection = selection; m_SelectionChanged.Emit(); }
}
void TextBox::EmitEdit(std::uint64_t previous)
{
	m_Impl->m_PreferredX.reset();
	m_Impl->m_Upstream = false;
	RefreshCaret();
	if (previous != m_Impl->m_Editor.m_Revision)
	{
		m_EditStateChanged.Emit();
		if (m_TextChanged.HasConnections()) m_TextChanged.Emit(GetText());
	}
}
void TextBox::CancelComposition()
{
	const bool active = IsComposing();
	m_Impl->m_Composition.Clear();
	m_Impl->m_DirtyVisual = true;
	if (active && GetWindow() && HasFocus())
	{
		GetWindow()->StopTextInput();
		if (!IsReadOnly()) GetWindow()->StartTextInput(GetBounds());
	}
	RefreshCaret();
}
void TextBox::SetSelection(std::size_t anchor, std::size_t caret)
{
	Dispatch([&] { if (IsComposing()) CancelComposition(); m_Impl->m_Editor.Select(anchor, caret); m_Impl->m_PreferredX.reset(); m_Impl->m_Upstream = false; RefreshCaret(); });
}
void TextBox::SelectAll() { SetSelection(0, GetText().size()); }
void TextBox::ReplaceSelection(std::string_view text)
{
	if (IsReadOnly()) return;
	Dispatch([&] { CancelComposition(); const auto previous = m_Impl->m_Editor.m_Revision; m_Impl->m_Editor.Insert(text); EmitEdit(previous); });
}
void TextBox::Undo()
{
	if (IsReadOnly()) return;
	Dispatch([&] { CancelComposition(); const auto previous = m_Impl->m_Editor.m_Revision; m_Impl->m_Editor.Undo(); EmitEdit(previous); });
}
void TextBox::Redo()
{
	if (IsReadOnly()) return;
	Dispatch([&] { CancelComposition(); const auto previous = m_Impl->m_Editor.m_Revision; m_Impl->m_Editor.Redo(); EmitEdit(previous); });
}
Result<void> TextBox::Copy()
{
	if (IsPassword()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Password fields cannot be copied."});
	if (IsComposing()) CancelComposition();
	if (m_Impl->m_Editor.m_Anchor == m_Impl->m_Editor.m_Caret) return {};
	if (!GetWindow()) return std::unexpected(Error{ErrorCode::PlatformOperation, "Clipboard access requires a window."});
	return GetWindow()->SetClipboard(m_Impl->m_Editor.GetSelection());
}
Result<void> TextBox::Cut()
{
	if (IsReadOnly()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Read-only text cannot be cut."});
	auto copied = Copy();
	if (copied) ReplaceSelection("");
	return copied;
}
Result<void> TextBox::Paste()
{
	if (IsReadOnly()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Read-only text cannot be pasted."});
	if (!GetWindow()) return std::unexpected(Error{ErrorCode::PlatformOperation, "Clipboard access requires a window."});
	auto text = GetWindow()->GetClipboard();
	if (!text) return std::unexpected(text.error());
	ReplaceSelection(*text);
	return {};
}
bool TextBox::Find(std::string_view pattern, bool matchCase, bool backwards, bool wrap)
{
	bool found = false;
	Dispatch([&]
	{
		CancelComposition();
		auto& editor = m_Impl->m_Editor;
		auto match = editor.Find(pattern, backwards ? editor.SelectionStart() : editor.SelectionEnd(), matchCase, backwards);
		if (!match && wrap) match = editor.Find(pattern, backwards ? GetText().size() : 0, matchCase, backwards);
		if (match) { editor.Select(match->first, match->second); m_Impl->m_Upstream = false; RefreshCaret(); found = true; }
	});
	return found;
}
bool TextBox::Replace(std::string_view pattern, std::string_view replacement, bool matchCase)
{
	if (IsReadOnly() || pattern.empty()) return false;
	bool replaced = false;
	Dispatch([&]
	{
		CancelComposition();
		auto& editor = m_Impl->m_Editor;
		auto match = editor.Find(pattern, editor.SelectionStart(), matchCase);
		if (!match || match->first != editor.SelectionStart() || match->second != editor.SelectionEnd())
		{
			if (!Find(pattern, matchCase)) return;
		}
		const auto previous = editor.m_Revision;
		editor.Insert(replacement);
		EmitEdit(previous);
		replaced = true;
	});
	return replaced;
}
std::size_t TextBox::ReplaceAll(std::string_view pattern, std::string_view replacement, bool matchCase)
{
	if (IsReadOnly()) return 0;
	std::size_t count = 0;
	Dispatch([&] { CancelComposition(); const auto previous = m_Impl->m_Editor.m_Revision; count = m_Impl->m_Editor.ReplaceAll(pattern, replacement, matchCase); EmitEdit(previous); });
	return count;
}
void TextBox::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	if (IsComposing()) CancelComposition();
	PrepareLayout();
	const auto local = ToLocal(event.m_Position);
	const auto vertical = VerticalThumb(), horizontal = HorizontalThumb();
	if (!vertical.Empty() && local.m_X >= vertical.m_X - 3)
	{
		m_Impl->m_DragAxis = 2;
		if (!vertical.Contains(local)) SetOffset({m_Impl->m_Scroll.m_X, m_Impl->m_Scroll.m_Y + (local.m_Y < vertical.m_Y ? -m_Impl->m_Viewport.m_Height : m_Impl->m_Viewport.m_Height)});
	}
	else if (!horizontal.Empty() && local.m_Y >= horizontal.m_Y - 3)
	{
		m_Impl->m_DragAxis = 1;
		if (!horizontal.Contains(local)) SetOffset({m_Impl->m_Scroll.m_X + (local.m_X < horizontal.m_X ? -m_Impl->m_Viewport.m_Width : m_Impl->m_Viewport.m_Width), m_Impl->m_Scroll.m_Y});
	}
	else
	{
		const auto offset = OffsetAt(event.m_Position);
		auto& editor = m_Impl->m_Editor;
		m_Impl->m_SelectionMode = std::min(3, event.m_ClickCount);
		if (m_Impl->m_SelectionMode == 3) editor.SelectLine(offset);
		else if (m_Impl->m_SelectionMode == 2) editor.SelectWord(offset);
		else editor.Select(event.m_Modifiers.m_Shift ? editor.m_Anchor : offset, offset);
		m_Impl->m_DragSelection = {editor.SelectionStart(), editor.SelectionEnd()};
		m_Impl->m_Dragging = true;
		m_Impl->m_PreferredX.reset();
		m_Impl->m_Upstream = false;
		RefreshCaret();
	}
	SetOffset(m_Impl->m_Scroll);
	m_Impl->m_DragStart = local;
	m_Impl->m_DragPointer = event.m_Position;
	m_Impl->m_DragOffset = m_Impl->m_Scroll;
	CapturePointer();
	event.m_Handled = true;
}
void TextBox::OnPointerMove(PointerEvent& event)
{
	if (!m_Impl->m_Dragging && !m_Impl->m_DragAxis) return;
	m_Impl->m_DragPointer = event.m_Position;
	const auto local = ToLocal(event.m_Position);
	if (m_Impl->m_DragAxis)
	{
		const auto extent = m_Impl->m_Document.GetSize();
		const auto viewport = m_Impl->m_Viewport;
		if (m_Impl->m_DragAxis == 2) SetOffset({m_Impl->m_Scroll.m_X, m_Impl->m_DragOffset.m_Y + (local.m_Y - m_Impl->m_DragStart.m_Y) * (extent.m_Height - viewport.m_Height) / std::max(1.0f, viewport.m_Height - VerticalThumb().m_Height)});
		else SetOffset({m_Impl->m_DragOffset.m_X + (local.m_X - m_Impl->m_DragStart.m_X) * (extent.m_Width - viewport.m_Width) / std::max(1.0f, viewport.m_Width - HorizontalThumb().m_Width), m_Impl->m_Scroll.m_Y});
	}
	else
	{
		const auto offset = OffsetAt(event.m_Position);
		auto& editor = m_Impl->m_Editor;
		if (m_Impl->m_SelectionMode > 1)
		{
			if (m_Impl->m_SelectionMode == 3) editor.SelectLine(offset); else editor.SelectWord(offset);
			editor.Select(offset < m_Impl->m_DragSelection.first ? m_Impl->m_DragSelection.second : m_Impl->m_DragSelection.first,
				offset < m_Impl->m_DragSelection.first ? editor.SelectionStart() : editor.SelectionEnd());
		}
		else editor.Select(editor.m_Anchor, offset);
		RefreshCaret();
		if (!m_Impl->m_Viewport.Contains(local)) RequestAnimationFrame();
		m_Impl->m_EnsureCaret = false;
	}
	event.m_Handled = true;
}
void TextBox::OnPointerUp(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left || (!m_Impl->m_Dragging && !m_Impl->m_DragAxis)) return;
	m_Impl->m_Dragging = false;
	m_Impl->m_DragAxis = 0;
	ReleasePointer();
	event.m_Handled = true;
}
void TextBox::OnPointerCaptureLost() { m_Impl->m_Dragging = false; m_Impl->m_DragAxis = 0; }
void TextBox::OnFocusChanged(bool focused)
{
	OnPointerCaptureLost();
	m_Impl->m_Editor.BreakUndoGroup();
	if (!focused) { ReleasePointer(); if (GetWindow()) GetWindow()->StopTextInput(); CancelComposition(); }
	else if (GetWindow() && !IsReadOnly()) GetWindow()->StartTextInput(GetCaretBounds());
	RefreshCaret();
}
void TextBox::OnTextEditing(std::string_view text, int start, int length)
{
	if (IsReadOnly()) return;
	m_Impl->m_Editor.BreakUndoGroup();
	m_Impl->m_Composition.Update(text, start, length, !m_Impl->m_Multiline);
	m_Impl->m_DirtyVisual = true;
	RefreshCaret();
}
void TextBox::OnTextInput(std::string_view text)
{
	if (IsReadOnly()) return;
	if (IsReadOnly()) return;
	const bool composition = IsComposing();
	m_Impl->m_Composition.Clear();
	m_Impl->m_DirtyVisual = true;
	const auto previous = m_Impl->m_Editor.m_Revision;
	m_Impl->m_Editor.Insert(text, !composition);
	EmitEdit(previous);
}
void TextBox::MoveVertical(int lines, bool select)
{
	PrepareLayout();
	auto& editor = m_Impl->m_Editor;
	const auto visual = m_Impl->m_Visual.ToVisual(editor.m_Caret);
	const auto line = m_Impl->m_Document.LineAt(visual, m_Impl->m_Upstream);
	if (!m_Impl->m_PreferredX) m_Impl->m_PreferredX = m_Impl->m_Document.CaretX(line, visual);
	const auto target = static_cast<std::size_t>(std::clamp(static_cast<std::int64_t>(line) + lines, std::int64_t(0), static_cast<std::int64_t>(m_Impl->m_Document.GetLines().size()) - 1));
	editor.MoveTo(m_Impl->m_Visual.ToSource(m_Impl->m_Document.Hit(target, *m_Impl->m_PreferredX)), select);
	m_Impl->m_Upstream = false;
}
void TextBox::OnKeyDown(KeyEvent& event)
{
	if (IsComposing()) { if (event.m_Key == Key::Escape) CancelComposition(); event.m_Handled = true; return; }
	if (event.m_Modifiers.m_Alt || event.m_Modifiers.m_Super) return;
	auto& editor = m_Impl->m_Editor;
	const auto previous = editor.m_Revision;
	const bool control = event.m_Modifiers.m_Control && !event.m_Modifiers.m_Alt && !event.m_Modifiers.m_AltGraph;
	const bool select = event.m_Modifiers.m_Shift;
	const bool vertical = m_Impl->m_Multiline && (event.m_Key == Key::Up || event.m_Key == Key::Down || event.m_Key == Key::PageUp || event.m_Key == Key::PageDown);
	if (!vertical) m_Impl->m_PreferredX.reset();
	bool handled = true;
	if (control && event.m_Key == Key::A) editor.SelectAll();
	else if ((control && event.m_Key == Key::C) || (control && event.m_Key == Key::Insert)) { auto result = Copy(); (void)result; event.m_Handled = true; return; }
	else if ((control && event.m_Key == Key::X) || (select && event.m_Key == Key::Delete)) { auto result = Cut(); (void)result; event.m_Handled = true; return; }
	else if ((control && event.m_Key == Key::V) || (select && event.m_Key == Key::Insert)) { auto result = Paste(); (void)result; event.m_Handled = true; return; }
	else if (control && event.m_Key == Key::Z && !IsReadOnly()) { if (select) editor.Redo(); else editor.Undo(); }
	else if (control && event.m_Key == Key::Y && !IsReadOnly()) editor.Redo();
	else if (event.m_Key == Key::Left) { editor.MoveLeft(select, control); m_Impl->m_Upstream = false; }
	else if (event.m_Key == Key::Right) { editor.MoveRight(select, control); m_Impl->m_Upstream = false; }
	else if (event.m_Key == Key::Home || event.m_Key == Key::End)
	{
		const bool end = event.m_Key == Key::End;
		if (!m_Impl->m_Multiline || control) editor.MoveTo(end ? editor.m_Text.size() : 0, select);
		else
		{
			PrepareLayout();
			const auto index = m_Impl->m_Document.LineAt(m_Impl->m_Visual.ToVisual(editor.m_Caret), m_Impl->m_Upstream);
			const auto& line = m_Impl->m_Document.GetLines()[index];
			editor.MoveTo(m_Impl->m_Visual.ToSource(end ? line.m_End : line.m_Start), select);
		}
		m_Impl->m_Upstream = end;
	}
	else if (vertical)
	{
		const int step = event.m_Key == Key::PageDown || event.m_Key == Key::PageUp ? std::max(1, static_cast<int>(m_Impl->m_Viewport.m_Height / m_Impl->m_Document.GetLineHeight()) - 1) : 1;
		MoveVertical((event.m_Key == Key::Up || event.m_Key == Key::PageUp) ? -step : step, select);
	}
	else if (event.m_Key == Key::Backspace && !IsReadOnly()) editor.Backspace(control);
	else if (event.m_Key == Key::Delete && !IsReadOnly()) editor.Delete(control);
	else if (event.m_Key == Key::Enter)
	{
		if (m_Impl->m_Multiline) { if (!IsReadOnly()) editor.Insert("\n"); }
		else { event.m_Handled = true; m_Submitted.Emit(); return; }
	}
	else handled = false;
	if (!handled) return;
	event.m_Handled = true;
	RefreshCaret();
	if (previous != editor.m_Revision)
	{
		m_Impl->m_Upstream = false;
		m_EditStateChanged.Emit();
		if (m_TextChanged.HasConnections()) m_TextChanged.Emit(GetText());
	}
}
}
