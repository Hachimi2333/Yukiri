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
	[[nodiscard]] const std::vector<std::string>& GetItems() const noexcept;
	void SetExpanded(bool expanded);
	[[nodiscard]] bool IsExpanded() const noexcept;
	void OnPointerDown(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	[[nodiscard]] bool AcceptsReturn() const noexcept override { return true; }
	Signal<int> m_SelectionChanged;
protected:
	bool OnAnimate(std::chrono::nanoseconds delta) override;
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
private:
	friend class Detail::AccessibilityTree;
	std::uint64_t m_ItemsGeneration = 0;
	void Open();
	void PaintPopup(Canvas& canvas, float progress);
	[[nodiscard]] Rect VisiblePopup() const;
	Detail::Transition m_Arrow;
	Detail::Transition m_HighlightVisual;
	float m_VisualProgress = 0;
	float m_WheelRemainder = 0;
	bool m_PopupUp = false;
	std::vector<std::string> m_Items;
	int m_Selected = -1;
	int m_Highlight = 0;
	int m_TopIndex = 0;
	int m_VisibleRows = 7;
	float m_RowHeight = 32;
	Rect m_PopupBounds;
};
}
