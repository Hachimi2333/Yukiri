#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Yukiri::Detail
{
class FramePacer
{
public:
	using Clock = std::chrono::steady_clock;
	using Time = Clock::time_point;
	void SetRefreshRate(int numerator, int denominator, float fallback = 0)
	{
		const double rate = numerator > 0 && denominator > 0 ? static_cast<double>(numerator) / denominator : fallback;
		m_Interval = std::isfinite(rate) && rate > 0 ? std::chrono::nanoseconds(static_cast<long long>(std::llround(1'000'000'000.0 / rate))) : std::chrono::nanoseconds::zero();
		m_Next = {};
	}
	void Submitted(Time start)
	{
		// Time spent recording or blocked in presentation is part of this interval.
		if (m_Next == Time{} || m_Interval <= std::chrono::nanoseconds::zero()) m_Next = start + m_Interval;
		else
		{
			const auto missed = start > m_Next ? (start - m_Next) / m_Interval : 0;
			m_Next += m_Interval * (missed + 1);
		}
	}
	void Retry(Time now) { m_Next = now + std::chrono::milliseconds(1); }
	void Reset() { m_Next = {}; }
	[[nodiscard]] Time GetDeadline() const noexcept { return m_Next; }
	[[nodiscard]] std::chrono::nanoseconds GetInterval() const noexcept { return m_Interval; }
	[[nodiscard]] bool IsDue(Time now) const noexcept { return now >= m_Next; }
private:
	std::chrono::nanoseconds m_Interval{};
	Time m_Next{};
};
}
