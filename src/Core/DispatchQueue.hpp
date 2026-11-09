#pragma once

#include <Yukiri/Core/Dispatcher.hpp>
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace Yukiri::Detail
{
struct TimerState
{
	std::atomic<bool> m_Active{true};
	std::chrono::nanoseconds m_Interval{};
	std::chrono::steady_clock::time_point m_Next;
	bool m_Repeat = false;
	std::move_only_function<void()> m_Callback;
};
struct DispatchState
{
	std::mutex m_Mutex;
	std::deque<std::move_only_function<void()>> m_Jobs;
	std::vector<std::shared_ptr<TimerState>> m_Timers;
	std::function<void()> m_Wake;
	std::thread::id m_Thread = std::this_thread::get_id();
	bool m_Stopped = false;
};
class DispatchQueue
{
public:
	using Clock = std::chrono::steady_clock;
	explicit DispatchQueue(std::function<void()> wake = {});
	~DispatchQueue();
	[[nodiscard]] Dispatcher GetDispatcher() const;
	void RunReady(Clock::time_point now = Clock::now());
	[[nodiscard]] Clock::time_point NextDeadline() const;
	void Stop();
private:
	std::shared_ptr<DispatchState> m_State;
	bool m_Running = false;
};
}
