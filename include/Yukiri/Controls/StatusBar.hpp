#pragma once
#include <Yukiri/Layout/StackPanel.hpp>

namespace Yukiri
{
class StatusBar : public StackPanel
{
public:
	StatusBar();
protected:
	void OnPaint(Canvas& canvas) override;
};
}
