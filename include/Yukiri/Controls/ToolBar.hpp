#pragma once
#include <Yukiri/Layout/WrapPanel.hpp>

namespace Yukiri
{
class ToolBar : public WrapPanel
{
public:
	ToolBar();
protected:
	void OnPaint(Canvas& canvas) override;
};
}
