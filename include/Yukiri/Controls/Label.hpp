#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <string>

namespace Yukiri
{
class Label : public Widget
{
public:
	explicit Label(std::string text = {});
	void SetText(std::string_view text);
	[[nodiscard]] const std::string& GetText() const noexcept;
	void SetWrap(bool wrap);
	void SetEllipsis(bool ellipsis);
	void SetTextAlignment(Alignment alignment);
protected:
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
private:
	[[nodiscard]] std::vector<std::string> Lines(float width) const;
	std::string m_Text;
	bool m_Wrap = false;
	bool m_Ellipsis = false;
	Alignment m_TextAlignment = Alignment::Start;
};
}
