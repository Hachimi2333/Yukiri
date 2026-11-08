#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

namespace Yukiri::Detail
{
class FrameStatistics
{
public:
	using Clock = std::chrono::steady_clock;
	bool m_Enabled = false;
	void Busy()
	{
		if (!m_Enabled) return;
		++m_Busy;
		if (m_BusyStart == Clock::time_point{}) m_BusyStart = Clock::now();
	}
	void Record(Clock::time_point start, double updateMs, double renderMs, double presentMs)
	{
		if (!m_Enabled) return;
		if (m_BusyStart != Clock::time_point{})
		{
			m_ResourceDeferred += std::chrono::duration<double, std::milli>(start - m_BusyStart).count();
			m_BusyStart = {};
		}
		const double interval = m_Last == Clock::time_point{} ? 0 : std::chrono::duration<double, std::milli>(start - m_Last).count();
		m_Last = start;
		if (!m_Count) m_ColdCpu = updateMs + renderMs;
		Sample sample{updateMs + renderMs, presentMs, interval, m_Count >= 10};
		if (m_Samples.size() < 8192) m_Samples.push_back(sample); else m_Samples[m_Count % 8192] = sample;
		++m_Count;
	}
	void Print(std::uint32_t id, std::chrono::nanoseconds period) const
	{
		if (!m_Enabled) return;
		std::vector<double> cpu, present, intervals;
		for (const auto& sample : m_Samples)
			if (sample.m_Warm) { cpu.push_back(sample.m_Cpu); present.push_back(sample.m_Present); intervals.push_back(sample.m_Interval); }
		auto percentile = [](std::vector<double> values, double fraction)
		{
			if (values.empty()) return 0.0;
			std::ranges::sort(values);
			return values[static_cast<std::size_t>(fraction * static_cast<double>(values.size() - 1))];
		};
		std::cout << "[Yukiri.Frame] window=" << id << " refresh_hz=" << (period.count() ? 1e9 / static_cast<double>(period.count()) : 0)
			<< " frames=" << m_Count << " cold_cpu_ms=" << m_ColdCpu << " warm_cpu_p50_ms=" << percentile(cpu, 0.5)
			<< " warm_cpu_p95_ms=" << percentile(cpu, 0.95) << " present_p95_ms=" << percentile(present, 0.95)
			<< " submit_interval_p50_ms=" << percentile(intervals, 0.5) << " submit_interval_p95_ms=" << percentile(intervals, 0.95)
			<< " resource_defer_ms=" << m_ResourceDeferred << " busy_retries=" << m_Busy << '\n';
	}
private:
	struct Sample { double m_Cpu; double m_Present; double m_Interval; bool m_Warm; };
	std::vector<Sample> m_Samples;
	std::uint64_t m_Count = 0;
	std::uint64_t m_Busy = 0;
	double m_ColdCpu = 0;
	double m_ResourceDeferred = 0;
	Clock::time_point m_BusyStart{};
	Clock::time_point m_Last{};
};
}
