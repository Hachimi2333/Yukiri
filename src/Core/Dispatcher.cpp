#include <Yukiri/Core/Dispatcher.hpp>
#include "DispatchQueue.hpp"
#include <algorithm>
#include <cassert>
#include <utility>

namespace Yukiri
{
Timer::Timer(std::shared_ptr<Detail::TimerState> state) : m_State(std::move(state)) {}
Timer::~Timer() { Stop(); }
Timer::Timer(Timer&& other) noexcept : m_State(std::move(other.m_State)) {}
Timer& Timer::operator=(Timer&& other) noexcept
{
	if (this != &other) { Stop(); m_State = std::move(other.m_State); }
	return *this;
}
void Timer::Stop() noexcept { if (m_State) m_State->m_Active = false; }
bool Timer::IsActive() const noexcept { return m_State && m_State->m_Active; }
Dispatcher::Dispatcher(std::weak_ptr<Detail::DispatchState> state) : m_State(std::move(state)) {}
bool Dispatcher::IsApplicationThread() const noexcept
{
	auto state = m_State.lock();
	return state && state->m_Thread == std::this_thread::get_id();
}
Result<void> Dispatcher::Post(std::move_only_function<void()> callback) const
{
	if (!callback) return std::unexpected(Error{ErrorCode::InvalidArgument, "A dispatch callback is required."});
	auto state = m_State.lock();
	if (!state) return std::unexpected(Error{ErrorCode::PlatformOperation, "The application dispatcher is stopped."});
	std::lock_guard lock(state->m_Mutex);
	if (state->m_Stopped) return std::unexpected(Error{ErrorCode::PlatformOperation, "The application dispatcher is stopped."});
	state->m_Jobs.push_back(std::move(callback));
	if (state->m_Wake) state->m_Wake();
	return {};
}
Result<Timer> Dispatcher::CreateTimer(std::chrono::nanoseconds interval, bool repeat, std::move_only_function<void()> callback) const
{
	if (interval <= std::chrono::nanoseconds::zero() || !callback)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "A timer requires a positive interval and a callback."});
	auto state = m_State.lock();
	if (!state) return std::unexpected(Error{ErrorCode::PlatformOperation, "The application dispatcher is stopped."});
	if (state->m_Thread != std::this_thread::get_id())
		return std::unexpected(Error{ErrorCode::InvalidArgument, "Timers must be created on the application thread."});
	auto timer = std::make_shared<Detail::TimerState>();
	timer->m_Interval = interval;
	const auto now = std::chrono::steady_clock::now();
	if (interval > std::chrono::steady_clock::time_point::max() - now)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "The timer interval exceeds the clock range."});
	timer->m_Next = now + interval;
	timer->m_Repeat = repeat;
	timer->m_Callback = std::move(callback);
	std::lock_guard lock(state->m_Mutex);
	if (state->m_Stopped) return std::unexpected(Error{ErrorCode::PlatformOperation, "The application dispatcher is stopped."});
	state->m_Timers.push_back(timer);
	if (state->m_Wake) state->m_Wake();
	return Timer(std::move(timer));
}
}

namespace Yukiri::Detail
{
DispatchQueue::DispatchQueue(std::function<void()> wake) : m_State(std::make_shared<DispatchState>()) { m_State->m_Wake = std::move(wake); }
DispatchQueue::~DispatchQueue() { Stop(); }
Dispatcher DispatchQueue::GetDispatcher() const { return Dispatcher(m_State); }
void DispatchQueue::Stop()
{
	std::deque<std::move_only_function<void()>> jobs;
	std::vector<std::shared_ptr<TimerState>> timers;
	{
		std::lock_guard lock(m_State->m_Mutex);
		m_State->m_Stopped = true;
		m_State->m_Wake = {};
		jobs.swap(m_State->m_Jobs);
		timers.swap(m_State->m_Timers);
		for (const auto& timer : timers) timer->m_Active = false;
	}
}
DispatchQueue::Clock::time_point DispatchQueue::NextDeadline() const
{
	std::lock_guard lock(m_State->m_Mutex);
	if (!m_State->m_Jobs.empty()) return Clock::time_point::min();
	auto deadline = Clock::time_point::max();
	for (const auto& timer : m_State->m_Timers)
		if (timer->m_Active) deadline = std::min(deadline, timer->m_Next);
	return deadline;
}
void DispatchQueue::RunReady(Clock::time_point now)
{
	assert(m_State->m_Thread == std::this_thread::get_id());
	if (m_Running) return;
	m_Running = true;
	struct Scope { bool& m_Value; ~Scope() { m_Value = false; } } scope{m_Running};
	std::deque<std::move_only_function<void()>> jobs;
	std::vector<std::shared_ptr<TimerState>> due;
	std::vector<std::shared_ptr<TimerState>> retired;
	{
		std::lock_guard lock(m_State->m_Mutex);
		for (int count = 0; count < 256 && !m_State->m_Jobs.empty(); ++count)
		{
			jobs.push_back(std::move(m_State->m_Jobs.front()));
			m_State->m_Jobs.pop_front();
		}
		for (const auto& timer : m_State->m_Timers)
		{
			if (!timer->m_Active || timer->m_Next > now) continue;
			due.push_back(timer);
			// Schedule relative to this tick, never replay an overdue backlog.
			timer->m_Next = timer->m_Interval > Clock::time_point::max() - now ? Clock::time_point::max() : now + timer->m_Interval;
		}
		std::erase_if(m_State->m_Timers, [&](const auto& timer)
		{
			if (timer->m_Active) return false;
			retired.push_back(timer);
			return true;
		});
	}
	for (auto& job : jobs)
	{
		{ std::lock_guard lock(m_State->m_Mutex); if (m_State->m_Stopped) break; }
		job();
	}
	for (const auto& timer : due)
	{
		if (!timer->m_Active) continue;
		if (!timer->m_Repeat) timer->m_Active = false;
		timer->m_Callback();
	}
}
}
