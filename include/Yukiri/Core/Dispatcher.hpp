#pragma once

#include <Yukiri/Core/Error.hpp>
#include <chrono>
#include <functional>
#include <memory>

namespace Yukiri
{
namespace Detail { class DispatchQueue; struct DispatchState; struct TimerState; }

class Timer
{
public:
	Timer() = default;
	~Timer();
	Timer(Timer&& other) noexcept;
	Timer& operator=(Timer&& other) noexcept;
	Timer(const Timer&) = delete;
	Timer& operator=(const Timer&) = delete;
	void Stop() noexcept;
	[[nodiscard]] bool IsActive() const noexcept;
private:
	friend class Dispatcher;
	explicit Timer(std::shared_ptr<Detail::TimerState> state);
	std::shared_ptr<Detail::TimerState> m_State;
};

class Dispatcher
{
public:
	Dispatcher() = default;
	[[nodiscard]] Result<void> Post(std::move_only_function<void()> callback) const;
	template<typename T, typename F>
	[[nodiscard]] Result<void> Post(std::weak_ptr<T> owner, F callback) const
	{
		return Post([owner = std::move(owner), callback = std::move(callback)]() mutable
		{
			if (auto alive = owner.lock()) std::invoke(callback, *alive);
		});
	}
	[[nodiscard]] Result<Timer> CreateTimer(std::chrono::nanoseconds interval, bool repeat,
		std::move_only_function<void()> callback) const;
	[[nodiscard]] bool IsApplicationThread() const noexcept;
private:
	friend class Detail::DispatchQueue;
	explicit Dispatcher(std::weak_ptr<Detail::DispatchState> state);
	std::weak_ptr<Detail::DispatchState> m_State;
};
}
