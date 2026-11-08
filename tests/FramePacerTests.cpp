#include "Platform/FramePacer.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

TEST_CASE("Frame deadlines follow fractional refresh rates and skip missed intervals", "[pacing]")
{
	for (int numerator : {59940, 60000, 120000, 144000, 165000, 240000})
	{
		Yukiri::Detail::FramePacer pacer;
		pacer.SetRefreshRate(numerator, 1000);
		const auto period = pacer.GetInterval();
		REQUIRE(std::chrono::duration<double>(period).count() == Catch::Approx(1000.0 / numerator).epsilon(0.000001));
		const auto start = Yukiri::Detail::FramePacer::Time{} + 1s;
		pacer.Submitted(start);
		REQUIRE_FALSE(pacer.IsDue(start + period / 2));
		REQUIRE(pacer.IsDue(start + period));
		pacer.Submitted(start + period * 5 + period / 2);
		REQUIRE(pacer.GetDeadline() == start + period * 6);
		// A whole interval spent in presentation must not add another interval of sleep.
		REQUIRE(pacer.IsDue(start + period * 6));
	}
}

TEST_CASE("Window pacing remains independent and resource retries are bounded", "[pacing]")
{
	Yukiri::Detail::FramePacer slow, fast;
	slow.SetRefreshRate(60, 1);
	fast.SetRefreshRate(240, 1);
	const auto start = Yukiri::Detail::FramePacer::Time{} + 1s;
	slow.Submitted(start);
	fast.Submitted(start);
	REQUIRE(fast.GetDeadline() < slow.GetDeadline());
	fast.Retry(start);
	REQUIRE(fast.GetDeadline() == start + 1ms);
	REQUIRE(slow.GetDeadline() > fast.GetDeadline());
	fast.Reset();
	REQUIRE(fast.IsDue(start));
	fast.SetRefreshRate(0, 0);
	REQUIRE(fast.GetInterval() == 0ns);
}
