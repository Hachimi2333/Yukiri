#pragma once

#include <Yukiri/UI/Widget.hpp>

namespace Yukiri
{
class ProgressBar : public Widget
{
public:
	explicit ProgressBar(float value = 0);
	void SetValue(float value);
	[[nodiscard]] float GetValue() const noexcept;
	void SetIndeterminate(bool indeterminate);
	[[nodiscard]] bool IsIndeterminate() const noexcept;
protected:
	bool OnAnimate(std::chrono::nanoseconds delta) override;
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
private:
	float m_Value = 0;
	Detail::Transition m_ValueVisual;
	bool m_Indeterminate = false;
	float m_Phase = 0;
};
}
