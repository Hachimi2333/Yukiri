#pragma once

#include <Yukiri/Layout/StackPanel.hpp>

namespace Yukiri
{
class SplitPanel : public Widget
{
public:
	explicit SplitPanel(Orientation orientation = Orientation::Horizontal);
	void SetContent(std::unique_ptr<Widget> first, std::unique_ptr<Widget> second);
	void SetRatio(float ratio);
	[[nodiscard]] float GetRatio() const noexcept;
	void SetMinimumSizes(float first, float second);
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerMove(PointerEvent& event) override;
	void OnPointerUp(PointerEvent& event) override;
	void OnPointerCaptureLost() override;
	void OnFocusChanged(bool focused) override;
	void OnKeyDown(KeyEvent& event) override;
	Signal<float> m_RatioChanged;
protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
	void OnPaintOverlay(Canvas& canvas) override;
	Rect GetFocusBounds() const noexcept override;
private:
	[[nodiscard]] float FirstSize(float available) const;
	[[nodiscard]] Rect Content() const noexcept;
	Orientation m_Orientation;
	float m_Ratio = 0.5f;
	float m_FirstMinimum = 40;
	float m_SecondMinimum = 40;
	float m_DragOffset = 0;
	bool m_Dragging = false;
	Rect m_Divider;
};
}
