#include "FileDialogTask.hpp"
#include <exception>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Yukiri::Detail
{
void DialogCancellation::Cancel()
{
	m_Canceled = true;
	std::lock_guard lock(m_Mutex);
	if (m_Cancel) m_Cancel();
}
void DialogCancellation::Install(std::function<void()> cancel)
{
	std::lock_guard lock(m_Mutex);
	m_Cancel = std::move(cancel);
	if (m_Canceled && m_Cancel) m_Cancel();
}
void DialogCancellation::Clear() { std::lock_guard lock(m_Mutex); m_Cancel = {}; }
bool DialogCancellation::IsCanceled() const noexcept { return m_Canceled; }
Result<std::unique_ptr<FileDialogTask>> FileDialogTask::Start(Dispatcher dispatcher, Backend backend, FileDialog::Callback callback)
{
	if (!dispatcher.IsApplicationThread() || !backend || !callback)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "File dialogs require the application thread, backend, and callback."});
	auto task = std::unique_ptr<FileDialogTask>(new FileDialogTask());
	task->m_State = std::make_shared<State>();
	task->m_State->m_Callback = std::move(callback);
	try
	{
		task->m_Worker = std::thread([state = task->m_State, dispatcher, backend = std::move(backend)]
		{
			Result<std::vector<std::filesystem::path>> result;
			try { result = backend(state->m_Cancellation); }
			catch (const std::exception& error) { result = std::unexpected(Error{ErrorCode::PlatformOperation, error.what()}); }
			catch (...) { result = std::unexpected(Error{ErrorCode::PlatformOperation, "The file dialog backend failed."}); }
			state->m_Cancellation.Clear();
			if (state->m_Cancellation.IsCanceled()) result = std::vector<std::filesystem::path>{};
			if (!dispatcher.Post([state, result = std::move(result)]() mutable
			{
				if (!state->m_Active.exchange(false)) return;
				if (state->m_Cancellation.IsCanceled()) result = std::vector<std::filesystem::path>{};
				auto callback = std::move(state->m_Callback);
				callback(std::move(result));
			})) state->m_Active = false;
			state->m_Done = true;
		});
	}
	catch (const std::exception& error)
	{
		task->m_State->m_Done = true;
		return std::unexpected(Error{ErrorCode::PlatformOperation, error.what()});
	}
	return task;
}
FileDialogTask::~FileDialogTask() { Shutdown(); }
bool FileDialogTask::IsBusy() const noexcept { return m_State && m_State->m_Active; }
void FileDialogTask::Cancel() { if (m_State) m_State->m_Cancellation.Cancel(); }
void FileDialogTask::Shutdown()
{
	if (!m_State) return;
	m_State->m_Active = false;
	Cancel();
	if (m_Worker.joinable())
	{
#if defined(_WIN32)
		// A modal COM dialog can synchronously message its owner while closing.
		// Service native messages while draining, without reentering Yukiri::Poll.
		bool quit = false;
		WPARAM quitCode = 0;
		while (!m_State->m_Done)
		{
			MSG message{};
			while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				if (message.message == WM_QUIT) { quit = true; quitCode = message.wParam; continue; }
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
			MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
		}
		if (quit) PostQuitMessage(static_cast<int>(quitCode));
#endif
		m_Worker.join();
	}
	m_State.reset();
}
}
