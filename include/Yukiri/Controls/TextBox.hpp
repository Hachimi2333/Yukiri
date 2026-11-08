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
	void SelectAll();
	[[nodiscard]] bool AcceptsTextInput() const noexcept override;
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerUp(PointerEvent& event) override;
	void OnPointerMove(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	void OnTextInput(std::string_view text) override;
	void OnTextEditing(std::string_view text, int start, int length) override;
	void OnFocusChanged(bool focused) override;
	Signal<std::string> m_TextChanged;
	Signal<> m_Submitted;

protected:
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_Impl;
	void RefreshCaret();
	void CancelComposition();
	[[nodiscard]] std::size_t OffsetAt(Point point) const;
};
}
