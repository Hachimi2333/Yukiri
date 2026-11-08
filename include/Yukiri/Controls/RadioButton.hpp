#pragma once

#include <Yukiri/Controls/CheckBox.hpp>

namespace Yukiri
{
class RadioButton : public CheckBox
{
public:
	explicit RadioButton(std::string text = {}, std::string group = "Default");
	void SetChecked(bool checked) override;
protected:
	void Activate() override;
	void OnPaint(Canvas& canvas) override;
private:
	std::string m_Group;
};
}
