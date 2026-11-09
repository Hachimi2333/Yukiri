#pragma once

#include <Yukiri/Controls/Dialog.hpp>
#include <functional>

namespace Yukiri
{
class MessageDialog
{
public:
	enum class Buttons { Ok, OkCancel, YesNo, YesNoCancel, SaveDiscardCancel };
	[[nodiscard]] static Result<Dialog*> Show(Window& window, std::string title, std::string text,
		Buttons buttons = Buttons::Ok, std::function<void(Dialog::Result)> completed = {});
};
}
