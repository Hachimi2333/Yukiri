#pragma once

#include <Yukiri/Core/Dispatcher.hpp>
#include <Yukiri/Platform/FileDialog.hpp>
#include <atomic>
#include <mutex>
#include <thread>

namespace Yukiri::Detail
{
class DialogCancellation
{
public:
	void Cancel();
	void Install(std::function<void()> cancel);
	void Clear();
	bool IsCanceled() const noexcept;
private:
	std::atomic<bool> m_Canceled{false};
	std::mutex m_Mutex;
	std::function<void()> m_Cancel;
};
class FileDialogTask
{
public:
	using Backend = std::function<Result<std::vector<std::filesystem::path>>(DialogCancellation&)>;
	static Result<std::unique_ptr<FileDialogTask>> Start(Dispatcher dispatcher, Backend backend, FileDialog::Callback callback);
	~FileDialogTask();
	void Cancel();
	void Shutdown();
	bool IsBusy() const noexcept;
private:
	struct State
	{
		DialogCancellation m_Cancellation;
		std::atomic<bool> m_Active{true};
		std::atomic<bool> m_Done{false};
		FileDialog::Callback m_Callback;
	};
	FileDialogTask() = default;
	std::shared_ptr<State> m_State;
	std::thread m_Worker;
};
enum class FileDialogMode { Open, Save, Folder };
FileDialogTask::Backend NativeFileDialog(void* owner, FileDialogMode mode, FileDialog::Config config);
}
