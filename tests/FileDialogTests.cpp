#include "Platform/FileDialogTask.hpp"
#include "Core/DispatchQueue.hpp"
#include <catch2/catch_test_macros.hpp>
#include <future>

namespace
{
void Drain(Yukiri::Detail::DispatchQueue& queue, Yukiri::Detail::FileDialogTask& task)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
	while (task.IsBusy() && std::chrono::steady_clock::now() < deadline)
	{
		queue.RunReady();
		std::this_thread::yield();
	}
	REQUIRE_FALSE(task.IsBusy());
}
}
TEST_CASE("File picker substitutes deliver success, cancellation, and errors on the UI thread", "[file-dialog]")
{
	Yukiri::Detail::DispatchQueue queue;
	const auto ui = std::this_thread::get_id();
	for (int mode = 0; mode < 3; ++mode)
	{
		int delivered = 0;
		auto task = Yukiri::Detail::FileDialogTask::Start(queue.GetDispatcher(), [mode, ui](auto&) -> Yukiri::Result<std::vector<std::filesystem::path>>
		{
			if (std::this_thread::get_id() == ui) return std::unexpected(Yukiri::Error{});
			if (mode == 2) return std::unexpected(Yukiri::Error{Yukiri::ErrorCode::PlatformOperation, "Simulated failure"});
			if (mode == 1) return std::vector<std::filesystem::path>{};
			return std::vector<std::filesystem::path>{L"example.txt"};
		}, [&](auto result)
		{
			REQUIRE(std::this_thread::get_id() == ui);
			REQUIRE(result.has_value() == (mode != 2));
			if (result) REQUIRE(result->size() == (mode == 0 ? 1 : 0));
			++delivered;
		});
		REQUIRE(task);
		Drain(queue, **task);
		REQUIRE(delivered == 1);
	}
}

TEST_CASE("File picker cancellation and destroyed owners discard late asynchronous results", "[file-dialog][lifetime]")
{
	Yukiri::Detail::DispatchQueue queue;
	for (bool destroy : {false, true})
	{
		int delivered = 0;
		std::promise<void> started;
		auto ready = started.get_future();
		auto task = Yukiri::Detail::FileDialogTask::Start(queue.GetDispatcher(), [&](auto& cancellation) -> Yukiri::Result<std::vector<std::filesystem::path>>
		{
			started.set_value();
			while (!cancellation.IsCanceled()) std::this_thread::yield();
			return std::vector<std::filesystem::path>{L"late.txt"};
		}, [&](auto result) { REQUIRE(result); REQUIRE(result->empty()); ++delivered; });
		REQUIRE(task);
		REQUIRE(ready.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
		if (destroy) task->reset();
		else { (*task)->Cancel(); Drain(queue, **task); }
		queue.RunReady();
		REQUIRE(delivered == (destroy ? 0 : 1));
	}
}
