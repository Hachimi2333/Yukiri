#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace Yukiri
{
class Connection
{
public:
	Connection() = default;
	explicit Connection(std::function<void()> disconnect) : m_Disconnect(std::move(disconnect)) {}
	~Connection() { Disconnect(); }
	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;
	Connection(Connection&& other) noexcept : m_Disconnect(std::exchange(other.m_Disconnect, {})) {}
	Connection& operator=(Connection&& other) noexcept
	{
		if (this != &other)
		{
			Disconnect();
			m_Disconnect = std::exchange(other.m_Disconnect, {});
		}
		return *this;
	}
	void Disconnect() noexcept
	{
		if (m_Disconnect)
		{
			auto disconnect = std::exchange(m_Disconnect, {});
			disconnect();
		}
	}

private:
	std::function<void()> m_Disconnect;
};

template<typename... Args>
class Signal
{
	struct Slot
	{
		std::function<void(Args...)> m_Callback;
		bool m_Connected = true;
	};

public:
	Signal() = default;
	~Signal() { DisconnectAll(); }
	Signal(const Signal&) = delete;
	Signal& operator=(const Signal&) = delete;
	[[nodiscard]] Connection Connect(std::function<void(Args...)> callback)
	{
		std::erase_if(m_Slots, [](const auto& slot) { return !slot->m_Connected; });
		auto slot = std::make_shared<Slot>(Slot{std::move(callback), true});
		m_Slots.push_back(slot);
		return Connection([weak = std::weak_ptr<Slot>(slot)]
		{
			if (auto connected = weak.lock()) connected->m_Connected = false;
		});
	}
	template<typename T, typename F>
	[[nodiscard]] Connection Connect(std::weak_ptr<T> owner, F callback)
	{
		return Connect([owner, callback = std::move(callback)](Args... args)
		{
			if (auto alive = owner.lock()) std::invoke(callback, *alive, args...);
		});
	}
	void Emit(Args... args)
	{
		const auto slots = m_Slots;
		for (const auto& slot : slots)
			if (slot->m_Connected) slot->m_Callback(args...);
	}
	void DisconnectAll() noexcept
	{
		for (const auto& slot : m_Slots) slot->m_Connected = false;
		m_Slots.clear();
	}
	[[nodiscard]] bool HasConnections() const noexcept
	{
		return std::ranges::any_of(m_Slots, [](const auto& slot) { return slot->m_Connected; });
	}

private:
	std::vector<std::shared_ptr<Slot>> m_Slots;
};
}
