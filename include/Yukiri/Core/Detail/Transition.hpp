#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Yukiri::Detail
{
// A value transition; logical control state remains separate from this visual state.
class Transition
{
public:
	explicit Transition(float value = 0) : m_Value(value), m_Start(value), m_Target(value) {}
	void Snap(float value) noexcept
	{
		m_Value = m_Start = m_Target = value;
		m_Elapsed = m_Duration = std::chrono::nanoseconds::zero();
	}
	void Retarget(float target, std::chrono::nanoseconds duration) noexcept
	{
		if (!std::isfinite(target)) return;
		if (duration <= std::chrono::nanoseconds::zero()) { Snap(target); return; }
		if (m_Target == target) return;
		m_Start = m_Value;
		m_Target = target;
		m_Duration = duration;
		m_Elapsed = std::chrono::nanoseconds::zero();
	}
	bool Advance(std::chrono::nanoseconds delta) noexcept
	{
		if (!IsActive()) return false;
		m_Elapsed += std::clamp(delta, std::chrono::nanoseconds::zero(), m_Duration - m_Elapsed);
		const float fraction = static_cast<float>(std::chrono::duration<double>(m_Elapsed).count() /
			std::chrono::duration<double>(m_Duration).count());
		const float remainder = 1 - fraction;
		m_Value = m_Elapsed == m_Duration ? m_Target : m_Start + (m_Target - m_Start) * (1 - remainder * remainder * remainder);
		return IsActive();
	}
	[[nodiscard]] bool IsActive() const noexcept { return m_Elapsed < m_Duration && m_Value != m_Target; }
	[[nodiscard]] float GetValue() const noexcept { return m_Value; }
	[[nodiscard]] float GetTarget() const noexcept { return m_Target; }
private:
	float m_Value;
	float m_Start;
	float m_Target;
	std::chrono::nanoseconds m_Elapsed{};
	std::chrono::nanoseconds m_Duration{};
};
}
