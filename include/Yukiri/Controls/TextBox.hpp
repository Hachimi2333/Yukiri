#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <string>

namespace Yukiri
{
class TextBox : public Widget
{
public:
	explicit TextBox(std::string text = {});
	~TextBox() override;
	void SetText(std::string_view text);
	[[nodiscard]] const std::string& GetText() const noexcept;
	void SetPlaceholder(std::string_view placeholder);
	void SetReadOnly(bool readOnly);
	[[nodiscard]] bool IsReadOnly() const noexcept;
	void SetPassword(bool password);
	[[nodiscard]] bool IsPassword() const noexcept;
	[[nodiscard]] std::string GetAccessibleText() const;
	[[nodiscard]] std::pair<std::size_t, std::size_t> GetSelection() const noexcept;
	void SetSelection(std::size_t anchor, std::size_t caret);
	void SelectAll();
	void ReplaceSelection(std::string_view text);
	void Undo();
	void Redo();
	[[nodiscard]] bool CanUndo() const noexcept;
	[[nodiscard]] bool CanRedo() const noexcept;
	[[nodiscard]] Result<void> Copy();
	[[nodiscard]] Result<void> Cut();
	[[nodiscard]] Result<void> Paste();
	bool Find(std::string_view pattern, bool matchCase = false, bool backwards = false, bool wrap = true);
	bool Replace(std::string_view pattern, std::string_view replacement, bool matchCase = false);
	std::size_t ReplaceAll(std::string_view pattern, std::string_view replacement, bool matchCase = false);
	void SetOffset(Point offset);
	[[nodiscard]] Point GetOffset() const noexcept;
	[[nodiscard]] Rect GetCaretBounds() const;
	[[nodiscard]] std::vector<Rect> GetTextBounds(std::size_t start, std::size_t end) const;
	[[nodiscard]] std::size_t HitTestOffset(Point global) const;
	[[nodiscard]] bool AcceptsTextInput() const noexcept override;
	[[nodiscard]] bool IsComposing() const noexcept override;
	[[nodiscard]] bool AcceptsReturn() const noexcept override;
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerUp(PointerEvent& event) override;
	void OnPointerCaptureLost() override;
	void OnPointerMove(PointerEvent& event) override;
	void OnWheel(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	void OnTextInput(std::string_view text) override;
	void OnTextEditing(std::string_view text, int start, int length) override;
	void OnFocusChanged(bool focused) override;
	Signal<std::string> m_TextChanged;
	Signal<> m_Submitted;
	Signal<> m_SelectionChanged;
	Signal<> m_EditStateChanged;

protected:
	TextBox(std::string text, bool multiline);
	void SetTextWrap(bool wrap);
	[[nodiscard]] bool GetTextWrap() const noexcept;
	bool OnAnimate(std::chrono::nanoseconds delta) override;
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
	void OnPaint(Canvas& canvas) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_Impl;
	void RefreshCaret();
	void CancelComposition();
	void PrepareLayout() const;
	void EnsureCaretVisible();
	void EmitEdit(std::uint64_t previous);
	void MoveVertical(int lines, bool select);
	void ScrollTo(Point offset);
	[[nodiscard]] Rect VerticalThumb() const;
	[[nodiscard]] Rect HorizontalThumb() const;
	[[nodiscard]] std::size_t OffsetAt(Point point) const;
};
}
