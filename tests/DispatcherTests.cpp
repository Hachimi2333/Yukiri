#include "Core/DispatchQueue.hpp"
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("Dispatch preserves order, defers nested posts, and protects expired owners", "[dispatch]")
{
	int wakes = 0;
	Yukiri::Detail::DispatchQueue queue([&] { ++wakes; });
	auto dispatcher = queue.GetDispatcher();
	std::vector<int> calls;
	bool workerPosted = false;
	bool workerIsUi = true;
	std::jthread worker([&]
	{
		workerIsUi = dispatcher.IsApplicationThread();
		auto posted = dispatcher.Post([&, value = std::make_unique<int>(1)]
		{
			calls.push_back(*value);
			REQUIRE(dispatcher.Post([&] { calls.push_back(3); }));
		});
		workerPosted = posted.has_value();
	});
	worker.join();
	REQUIRE(workerPosted);
	REQUIRE_FALSE(workerIsUi);
	REQUIRE(dispatcher.Post([&] { calls.push_back(2); }));
	auto owner = std::make_shared<int>(0);
	REQUIRE(dispatcher.Post(std::weak_ptr(owner), [&](int&) { calls.push_back(99); }));
	owner.reset();
	queue.RunReady();
	REQUIRE(calls == std::vector<int>{1, 2});
	queue.RunReady();
	REQUIRE(calls == std::vector<int>{1, 2, 3});
	REQUIRE(wakes == 4);
	queue.Stop();
	REQUIRE_FALSE(dispatcher.Post([] {}));
}

TEST_CASE("Timers use deadlines, skip missed ticks, and cancel safely in callbacks", "[dispatch]")
{
	Yukiri::Detail::DispatchQueue queue;
	auto dispatcher = queue.GetDispatcher();
	int ticks = 0;
	auto repeat = dispatcher.CreateTimer(10ms, true, [&] { ++ticks; });
	REQUIRE(repeat);
	const auto first = queue.NextDeadline();
	queue.RunReady(first - 1ns);
	REQUIRE(ticks == 0);
	queue.RunReady(first + 2s);
	REQUIRE(ticks == 1);
	REQUIRE(queue.NextDeadline() > first + 2s);
	repeat->Stop();
	auto once = dispatcher.CreateTimer(1ms, false, [&] { ++ticks; });
	REQUIRE(once);
	queue.RunReady(queue.NextDeadline());
	REQUIRE_FALSE(once->IsActive());
	REQUIRE(ticks == 2);
	auto self = dispatcher.CreateTimer(1ms, true, [&] { repeat->Stop(); });
	REQUIRE(self);
	*repeat = std::move(*self);
	queue.RunReady(queue.NextDeadline());
	REQUIRE_FALSE(repeat->IsActive());
	queue.Stop();
	REQUIRE_FALSE(dispatcher.CreateTimer(1ms, false, [] {}));
}

TEST_CASE("Dispatch shutdown cancels detached batches and rejects invalid timers", "[dispatch]")
{
	Yukiri::Dispatcher detached;
	REQUIRE_FALSE(detached.Post([] {}));
	Yukiri::Detail::DispatchQueue queue;
	auto dispatcher = queue.GetDispatcher();
	REQUIRE_FALSE(dispatcher.CreateTimer(0ns, true, [] {}));
	int calls = 0;
	REQUIRE(dispatcher.Post([&] { queue.Stop(); }));
	REQUIRE(dispatcher.Post([&] { ++calls; }));
	queue.RunReady();
	REQUIRE(calls == 0);
}
