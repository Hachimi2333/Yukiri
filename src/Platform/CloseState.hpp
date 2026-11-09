#pragma once

#include <Yukiri/Platform/Window.hpp>

namespace Yukiri::Detail
{
struct CloseState
{
	bool m_Open = true;
	bool m_Pending = false;
	bool m_Notified = false;
	void Request() noexcept { if (m_Open) m_Pending = true; }
	void Process(Signal<CloseRequestEvent&>& requested)
	{
		if (!m_Open || !std::exchange(m_Pending, false)) return;
		CloseRequestEvent event;
		requested.Emit(event);
		if (!event.m_Cancel) { m_Open = false; m_Pending = false; }
	}
};
}
