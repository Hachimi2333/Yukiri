#pragma once

#include <Yukiri/Controls/Button.hpp>

namespace Yukiri
{
class CheckBox : public Button
{
public:
	explicit CheckBox(std::string text = {}, bool checked = false);
	virtual void SetChecked(bool checked);
	[[nodiscard]] bool IsChecked() const noexcept;
	Signal<bool> m_CheckedChanged;
protected:
	bool OnAnimate(std::chrono::nanoseconds delta) override;
	Detail::Transition m_CheckVisual;
	Size m_LabelSize;
	Rect GetFocusBounds() const noexcept override;
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
	void Activate() override;
	void OnCommandChecked(bool checkable, bool checked) override;
private:
	bool m_Checked;
};
}
