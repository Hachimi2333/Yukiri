#include "WidgetTree.hpp"
#include <Yukiri/Controls/ToolTip.hpp>
#include <Yukiri/Platform/Window.hpp>

namespace Yukiri::Detail
{
void WidgetTree::HideToolTip()
{
	m_ToolTipTimer.Stop();
	m_ToolTipOwner = nullptr;
	auto* popup = std::exchange(m_ToolTipPopup, nullptr);
	if (popup && !m_ToolTipLifetime.expired()) ClosePopupFor(popup, true);
	m_ToolTipLifetime.reset();
}
void WidgetTree::UpdateToolTip()
{
	HideToolTip();
	if (!m_Window || !m_Hover || m_Capture) return;
	for (const auto& popup : m_Popups) if (!popup->m_Closing && popup->m_Interactive) return;
	auto* owner = m_Hover;
	while (owner && owner->GetToolTip().empty()) owner = owner->GetParent();
	if (!owner) return;
	m_ToolTipOwner = owner;
	const auto lifetime = owner->ObserveLifetime();
	auto timer = m_Window->GetDispatcher().CreateTimer(std::chrono::milliseconds(500), false, [this, owner, lifetime]
	{
		if (lifetime.expired() || m_ToolTipOwner != owner) return;
		const auto point = owner->ToLocal(m_PointerPosition);
		auto shown = ToolTip::Show(*owner, std::string(owner->GetToolTip()), {point.m_X + 12, point.m_Y + 18});
		if (shown)
		{
			m_ToolTipPopup = *shown;
			m_ToolTipLifetime = (*shown)->ObserveLifetime();
		}
	});
	if (timer) m_ToolTipTimer = std::move(*timer);
}
}
