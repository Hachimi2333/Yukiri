#include <Yukiri/Core/Signal.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Signal connections survive reentrant subscription changes", "[signal]")
{
	Yukiri::Signal<int> signal;
	int first = 0;
	int second = 0;
	Yukiri::Connection later;
	auto initial = signal.Connect([&](int value)
	{
		first += value;
		if (first == 1) later = signal.Connect([&](int next) { second += next; });
	});
	signal.Emit(1);
	REQUIRE(first == 1);
	REQUIRE(second == 0);
	signal.Emit(2);
	REQUIRE(first == 3);
	REQUIRE(second == 2);
	initial.Disconnect();
	signal.Emit(3);
	REQUIRE(first == 3);
	REQUIRE(second == 5);
}

TEST_CASE("Signal disconnects pending callbacks and expired owners", "[signal]")
{
	Yukiri::Signal<> signal;
	Yukiri::Connection second;
	auto first = signal.Connect([&] { second.Disconnect(); });
	bool called = false;
	second = signal.Connect([&] { called = true; });
	signal.Emit();
	REQUIRE_FALSE(called);
	auto owner = std::make_shared<int>(0);
	auto guarded = signal.Connect(std::weak_ptr<int>(owner), [](int& count) { ++count; });
	signal.Emit();
	REQUIRE(*owner == 1);
	owner.reset();
	signal.Emit();
}

TEST_CASE("Connection can outlive its signal", "[signal]")
{
	Yukiri::Connection connection;
	{
		Yukiri::Signal<> signal;
		connection = signal.Connect([] {});
	}
	connection.Disconnect();
}
