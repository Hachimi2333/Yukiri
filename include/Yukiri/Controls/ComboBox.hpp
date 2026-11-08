#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/UI/Widget.hpp>
#include <string>

namespace Yukiri
{
class ComboBox : public Widget
{
public:
	explicit ComboBox(std::vector<std::string> items = {});
	void SetItems(std::vector<std::string> items);
	[[nodiscard]] Result<void> SetSelectedIndex(int index);
	[[nodiscard]] int GetSelectedIndex() const noexcept;
	[[nodiscard]] std::string_view GetSelectedText() const noexcept;
	void OnPointerDown(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	Signal<int> m_SelectionChanged;
protected:
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
private:
	void Open();
	void PaintPopup(Canvas& canvas);
	std::vector<std::string> m_Items;
	int m_Selected = -1;
	int m_Highlight = 0;
	int m_TopIndex = 0;
	Rect m_PopupBounds;
};
}
