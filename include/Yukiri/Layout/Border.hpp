#pragma once

#include <Yukiri/UI/Widget.hpp>

namespace Yukiri
{
class Border : public Widget
{
public:
	Border();
	Widget& SetContent(std::unique_ptr<Widget> content);
protected:
	void OnPaint(Canvas& canvas) override;
};
}
