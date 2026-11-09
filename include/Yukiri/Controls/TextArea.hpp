#pragma once

#include <Yukiri/Controls/TextBox.hpp>

namespace Yukiri
{
class TextArea : public TextBox
{
public:
	explicit TextArea(std::string text = {}) : TextBox(std::move(text), true) {}
	void SetWrap(bool wrap) { SetTextWrap(wrap); }
	[[nodiscard]] bool GetWrap() const noexcept { return GetTextWrap(); }
};
}
