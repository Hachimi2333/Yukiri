#pragma once

#include <chrono>

namespace Yukiri
{
struct MotionSettings
{
	bool m_Enabled = true;
	std::chrono::milliseconds m_StateDuration{120};
	std::chrono::milliseconds m_SelectionDuration{160};
	std::chrono::milliseconds m_PopupDuration{120};
	std::chrono::milliseconds m_ScrollDuration{140};
};
}
