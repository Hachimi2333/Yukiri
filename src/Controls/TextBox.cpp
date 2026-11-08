#include <Yukiri/Controls/TextBox.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/TextEditor.hpp"
#include <chrono>

namespace Yukiri
{
namespace
{
std::uint64_t Milliseconds()
{
	return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
}
std::size_t CharacterOffset(std::string_view text, int characters)
{
	const auto points = Detail::DecodeUtf8(text);
	if (characters <= 0) return 0;
	return static_cast<std::size_t>(characters) >= points.size() ? text.size() : points[static_cast<std::size_t>(characters)].m_Start;
}
}
struct TextBox::Impl
{
	Detail::TextEditor m_Editor;
	std::string m_Placeholder;
	std::string m_Composition;
	std::size_t m_CompositionCaret = 0;
	std::size_t m_CompositionLength = 0;
	bool m_ReadOnly = false;
	bool m_Dragging = false;
	float m_Scroll = 0;
	std::uint64_t m_LastInteraction = Milliseconds();
};
TextBox::TextBox(std::string text) : m_Impl(std::make_unique<Impl>())
{
	m_Impl->m_Editor.SetText(text);
	SetFocusable(true);
	SetCursor(Cursor::Text);
}
TextBox::~TextBox() { if (HasFocus() && GetWindow()) GetWindow()->StopTextInput(); }
const std::string& TextBox::GetText() const noexcept { return m_Impl->m_Editor.m_Text; }
void TextBox::SetText(std::string_view text)
{
	if (text == GetText()) return;
	m_Impl->m_Editor.SetText(text);
	m_Impl->m_Composition.clear();
	RefreshCaret();
	m_TextChanged.Emit(GetText());
}
void TextBox::SetPlaceholder(std::string_view placeholder) { m_Impl->m_Placeholder = Detail::SanitizeUtf8(placeholder, true); InvalidatePaint(); }
void TextBox::SetReadOnly(bool readOnly)
{
	m_Impl->m_ReadOnly = readOnly;
	if (readOnly) CancelComposition();
	else if (HasFocus() && GetWindow()) GetWindow()->StartTextInput(GetBounds());
	InvalidatePaint();
}
void TextBox::SelectAll() { m_Impl->m_Editor.SelectAll(); RefreshCaret(); }
bool TextBox::AcceptsTextInput() const noexcept { return !m_Impl->m_ReadOnly; }
Size TextBox::OnMeasure(Size) { return {220, std::max(GetTheme().m_ControlHeight, GetFontSize() * 1.5f + 12)}; }
void TextBox::RefreshCaret()
{
	m_Impl->m_LastInteraction = Milliseconds();
	InvalidatePaint();
}
void TextBox::CancelComposition()
{
	m_Impl->m_Composition.clear();
	if (GetWindow() && HasFocus())
	{
		GetWindow()->StopTextInput();
		if (HasFocus() && !m_Impl->m_ReadOnly) GetWindow()->StartTextInput(GetBounds());
	}
	RefreshCaret();
}
std::size_t TextBox::OffsetAt(Point point) const
{
	return GetWindow() ? GetWindow()->HitTestText(GetText(), ToLocal(point).m_X - 10 + m_Impl->m_Scroll, GetFontSize()) : 0;
}
void TextBox::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	if (!m_Impl->m_Composition.empty()) CancelComposition();
	const auto offset = OffsetAt(event.m_Position);
	m_Impl->m_Editor.Select(event.m_Modifiers.m_Shift ? m_Impl->m_Editor.m_Anchor : offset, offset);
	m_Impl->m_Dragging = true;
	CapturePointer();
	RefreshCaret();
	event.m_Handled = true;
}
void TextBox::OnPointerMove(PointerEvent& event)
{
	if (!m_Impl->m_Dragging) return;
	m_Impl->m_Editor.Select(m_Impl->m_Editor.m_Anchor, OffsetAt(event.m_Position));
	RefreshCaret();
	event.m_Handled = true;
}
void TextBox::OnPointerUp(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left || !m_Impl->m_Dragging) return;
	m_Impl->m_Dragging = false;
	ReleasePointer();
	event.m_Handled = true;
}
void TextBox::OnFocusChanged(bool focused)
{
	m_Impl->m_Dragging = false;
	if (!focused)
	{
		ReleasePointer();
		if (GetWindow()) GetWindow()->StopTextInput();
		CancelComposition();
	}
	else if (GetWindow() && !m_Impl->m_ReadOnly) GetWindow()->StartTextInput(GetBounds());
	RefreshCaret();
}
void TextBox::OnTextEditing(std::string_view text, int start, int length)
{
	if (m_Impl->m_ReadOnly) return;
	m_Impl->m_Composition = Detail::SanitizeUtf8(text, true);
	m_Impl->m_CompositionCaret = CharacterOffset(m_Impl->m_Composition, start);
	m_Impl->m_CompositionLength = CharacterOffset(m_Impl->m_Composition, start + std::max(0, length)) - m_Impl->m_CompositionCaret;
	RefreshCaret();
}
void TextBox::OnTextInput(std::string_view text)
{
	if (m_Impl->m_ReadOnly) return;
	m_Impl->m_Composition.clear();
	m_Impl->m_Editor.Insert(text);
	RefreshCaret();
	m_TextChanged.Emit(GetText());
}
void TextBox::OnKeyDown(KeyEvent& event)
{
	if (!m_Impl->m_Composition.empty())
	{
		if (event.m_Key == Key::Escape) CancelComposition();
		event.m_Handled = true;
		return;
	}
	auto& editor = m_Impl->m_Editor;
	const std::string previous = editor.m_Text;
	const bool control = event.m_Modifiers.m_Control && !event.m_Modifiers.m_Alt;
	const bool select = event.m_Modifiers.m_Shift;
	bool handled = true;
	if (control && event.m_Key == Key::A) editor.SelectAll();
	else if (control && (event.m_Key == Key::C || event.m_Key == Key::X))
	{
		if (GetWindow() && editor.m_Caret != editor.m_Anchor)
		{
			auto copied = GetWindow()->SetClipboard(editor.GetSelection());
			if (copied && event.m_Key == Key::X && !m_Impl->m_ReadOnly) editor.Insert("");
		}
	}
	else if (control && event.m_Key == Key::V && !m_Impl->m_ReadOnly && GetWindow())
	{
		auto pasted = GetWindow()->GetClipboard();
		if (pasted) editor.Insert(*pasted);
	}
	else if (control && event.m_Key == Key::Z && !m_Impl->m_ReadOnly) { if (select) editor.Redo(); else editor.Undo(); }
	else if (control && event.m_Key == Key::Y && !m_Impl->m_ReadOnly) editor.Redo();
	else if (event.m_Key == Key::Left) editor.MoveLeft(select, control);
	else if (event.m_Key == Key::Right) editor.MoveRight(select, control);
	else if (event.m_Key == Key::Home) editor.MoveTo(0, select);
	else if (event.m_Key == Key::End) editor.MoveTo(editor.m_Text.size(), select);
	else if (event.m_Key == Key::Backspace && !m_Impl->m_ReadOnly) editor.Backspace(control);
	else if (event.m_Key == Key::Delete && !m_Impl->m_ReadOnly) editor.Delete(control);
	else if (event.m_Key == Key::Enter)
	{
		event.m_Handled = true;
		m_Submitted.Emit();
		return;
	}
	else handled = false;
	if (!handled) return;
	event.m_Handled = true;
	RefreshCaret();
	if (editor.m_Text != previous) m_TextChanged.Emit(editor.m_Text);
}
void TextBox::OnPaint(Canvas& canvas)
{
	const auto size = GetActualSize();
	const auto& theme = GetTheme();
	canvas.FillRectangle(GetLocalBounds(), InteractiveColor(GetStyle().m_Background.value_or(theme.m_Surface), theme.m_Hover, theme.m_Pressed, false), GetRadius());
	canvas.StrokeRectangle(GetLocalBounds(), IsFocusVisible() ? theme.m_Accent : GetStyle().m_Border.value_or(theme.m_Border), IsFocusVisible() ? 1.5f : 1, GetRadius());
	auto& editor = m_Impl->m_Editor;
	std::string display = editor.m_Text;
	std::size_t caret = editor.m_Caret;
	const bool composing = !m_Impl->m_Composition.empty();
	if (composing)
	{
		display = editor.m_Text.substr(0, editor.SelectionStart()) + m_Impl->m_Composition + editor.m_Text.substr(editor.SelectionEnd());
		caret = editor.SelectionStart() + m_Impl->m_CompositionCaret;
	}
	const float textHeight = canvas.MeasureText(display, GetFontSize()).m_Height;
	const float top = (size.m_Height - textHeight) * 0.5f;
	const float caretX = GetWindow() ? GetWindow()->GetTextCaretX(display, caret, GetFontSize()) : 0;
	const float viewport = std::max(1.0f, size.m_Width - 22);
	if (caretX < m_Impl->m_Scroll) m_Impl->m_Scroll = caretX;
	if (caretX > m_Impl->m_Scroll + viewport) m_Impl->m_Scroll = caretX - viewport;
	m_Impl->m_Scroll = std::clamp(m_Impl->m_Scroll, 0.0f, std::max(0.0f, canvas.MeasureText(display, GetFontSize()).m_Width - viewport));
	const float left = 10 - m_Impl->m_Scroll;
	canvas.PushClip({8, 4, std::max(0.0f, size.m_Width - 16), std::max(0.0f, size.m_Height - 8)});
	if (!composing && editor.m_Caret != editor.m_Anchor && GetWindow())
	{
		const float start = GetWindow()->GetTextCaretX(display, editor.SelectionStart(), GetFontSize());
		const float end = GetWindow()->GetTextCaretX(display, editor.SelectionEnd(), GetFontSize());
		canvas.FillRectangle({left + start, top, end - start, textHeight}, theme.m_Selection, 2);
	}
	if (display.empty() && !HasFocus()) canvas.DrawText(m_Impl->m_Placeholder, {10, top}, theme.m_MutedText, GetFontSize());
	else canvas.DrawText(display, {left, top}, GetForeground(), GetFontSize());
	if (composing && GetWindow())
	{
		const float start = GetWindow()->GetTextCaretX(display, editor.SelectionStart(), GetFontSize());
		const float end = GetWindow()->GetTextCaretX(display, editor.SelectionStart() + m_Impl->m_Composition.size(), GetFontSize());
		canvas.FillRectangle({left + start, top + textHeight - 1, end - start, 1}, theme.m_Accent);
	}
	if (HasFocus() && (Milliseconds() - m_Impl->m_LastInteraction) / 500 % 2 == 0)
		canvas.FillRectangle({left + caretX, top + 1, 1.25f, textHeight - 2}, theme.m_Text);
	canvas.PopClip();
	if (HasFocus() && GetWindow() && !m_Impl->m_ReadOnly)
	{
		const auto bounds = GetBounds();
		GetWindow()->StartTextInput({bounds.m_X + left + caretX, bounds.m_Y + top, 1, textHeight});
	}
}
}
