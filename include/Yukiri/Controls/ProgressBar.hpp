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
protected:
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
private:
	float m_Value = 0;
};
}
