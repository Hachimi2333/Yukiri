#include <Yukiri/Controls/ToolTip.hpp>
#include <Yukiri/Controls/Label.hpp>
#include <Yukiri/Platform/Window.hpp>

namespace Yukiri
{
ToolTip::ToolTip(std::string text)
{
	SetPadding(Insets(8));
	SetMaxSize({320, std::numeric_limits<float>::infinity()});
	SetHitTestVisible(false);
	auto label = std::make_unique<Label>(std::move(text));
	label->SetWrap(true);
	SetContent(std::move(label));
}
Result<ToolTip*> ToolTip::Show(Widget& owner, std::string text, Point localPosition)
{
	if (!owner.GetWindow()) return std::unexpected(Error{ErrorCode::InvalidArgument, "The tooltip owner must belong to a window."});
	auto tooltip = std::make_unique<ToolTip>(std::move(text));
	auto* observer = tooltip.get();
	auto shown = owner.GetWindow()->OpenPopup(&owner, std::move(tooltip), {localPosition.m_X, localPosition.m_Y, 0, 0}, false, false, false);
	if (!shown) return std::unexpected(shown.error());
	return observer;
}
}
