#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/Core/Error.hpp>

namespace Yukiri
{
class Slider : public Widget
{
public:
	explicit Slider(float value = 0);
	[[nodiscard]] Result<void> SetRange(float minimum, float maximum);
	void SetValue(float value);
	[[nodiscard]] float GetValue() const noexcept;
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerMove(PointerEvent& event) override;
	void OnPointerUp(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	void OnFocusChanged(bool focused) override;
	Signal<float> m_ValueChanged;
protected:
	bool OnAnimate(std::chrono::nanoseconds delta) override;
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
private:
	void SetFromPoint(Point point);
	float m_Minimum = 0;
	float m_Maximum = 1;
	float m_Value = 0;
	Detail::Transition m_ValueVisual;
	bool m_Dragging = false;
};
}
