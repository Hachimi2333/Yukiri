#pragma once

#include <Yukiri/UI/Widget.hpp>

namespace Yukiri
{
class ScrollView : public Widget
{
public:
	ScrollView();
	Widget& SetContent(std::unique_ptr<Widget> content);
	void SetHorizontalScrollEnabled(bool enabled);
	void SetVerticalScrollEnabled(bool enabled);
	void SetOffset(Point offset);
	void SetSmoothScrolling(bool enabled);
	[[nodiscard]] Point GetOffset() const noexcept;
	void OnWheel(PointerEvent& event) override;
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerMove(PointerEvent& event) override;
	void OnPointerUp(PointerEvent& event) override;
	void OnPointerCaptureLost() override;
	void OnKeyDown(KeyEvent& event) override;
	void OnFocusChanged(bool focused) override;
protected:
	bool OnAnimate(std::chrono::nanoseconds delta) override;
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
	void OnPaintOverlay(Canvas& canvas) override;
	void OnBringIntoView(Rect& rectangle) override;
	[[nodiscard]] Rect GetChildrenClip() const noexcept override;
private:
	friend class Detail::AccessibilityTree;
	[[nodiscard]] Rect VerticalThumb() const;
	[[nodiscard]] Rect HorizontalThumb() const;
	[[nodiscard]] Point ClampOffset(Point offset) const;
	void ApplyOffset(Point offset);
	void ScrollTo(Point target);
	Point m_Offset;
	Point m_Target;
	Detail::Transition m_ScrollX;
	Detail::Transition m_ScrollY;
	bool m_Smooth = true;
	Size m_Content;
	Size m_Viewport;
	bool m_Horizontal = false;
	bool m_Vertical = true;
	int m_DragAxis = 0;
	Point m_DragStart;
	Point m_DragOffset;
};
}
