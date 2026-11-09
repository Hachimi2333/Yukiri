#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <string>

namespace Yukiri
{
class Button : public Widget
{
public:
	explicit Button(std::string text = {});
	void SetText(std::string_view text);
	[[nodiscard]] const std::string& GetText() const noexcept;
	void SetAccent(bool accent);
	void SetCommand(std::shared_ptr<Command> command);
	[[nodiscard]] std::shared_ptr<Command> GetCommand() const noexcept;
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerUp(PointerEvent& event) override;
	void OnPointerMove(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	void OnKeyUp(KeyEvent& event) override;
	void OnPointerCaptureLost() override;
	void OnFocusChanged(bool focused) override;
	Signal<> m_Clicked;
protected:
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
	virtual void Activate();
	bool m_Pressed = false;
	bool m_Armed = false;
private:
	std::string m_Text;
	bool m_Accent = false;
	bool m_KeyboardPressed = false;
	std::shared_ptr<Command> m_Command;
	Connection m_CommandChanged;
};
}
