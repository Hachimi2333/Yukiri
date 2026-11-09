#pragma once
#include <Yukiri/Layout/Border.hpp>

namespace Yukiri
{
class ToolTip : public Border
{
public:
	explicit ToolTip(std::string text);
	[[nodiscard]] static Result<ToolTip*> Show(Widget& owner, std::string text, Point localPosition);
};
}
