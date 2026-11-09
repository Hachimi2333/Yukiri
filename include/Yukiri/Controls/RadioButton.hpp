#pragma once

#include <Yukiri/Controls/CheckBox.hpp>

namespace Yukiri
{
class RadioButton : public CheckBox
{
public:
	explicit RadioButton(std::string text = {}, std::string group = "Default");
	void SetChecked(bool checked) override;
	void OnKeyDown(KeyEvent& event) override;
	[[nodiscard]] std::string_view GetGroup() const noexcept;
	[[nodiscard]] bool IsTabStop() const noexcept override;
protected:
	void OnArrange(Rect content) override;
	void Activate() override;
	void OnPaint(Canvas& canvas) override;
private:
	std::string m_Group;
};
}
