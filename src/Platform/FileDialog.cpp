#include <Yukiri/Platform/FileDialog.hpp>
#include "WindowState.hpp"
#include "FileDialogTask.hpp"

namespace Yukiri
{
Result<void> FileDialog::Open(Window& owner, Config config, Callback callback) { return Show(owner, Mode::Open, std::move(config), std::move(callback)); }
Result<void> FileDialog::Save(Window& owner, Config config, Callback callback) { return Show(owner, Mode::Save, std::move(config), std::move(callback)); }
Result<void> FileDialog::SelectFolder(Window& owner, Config config, Callback callback) { return Show(owner, Mode::Folder, std::move(config), std::move(callback)); }
Result<void> FileDialog::Show(Window& owner, Mode mode, Config config, Callback callback)
{
	if (!owner.IsOpen() || !owner.GetDispatcher().IsApplicationThread())
		return std::unexpected(Error{ErrorCode::InvalidArgument, "A file dialog requires an open owner on the application thread."});
	if (owner.m_Impl->m_FileDialog && owner.m_Impl->m_FileDialog->IsBusy())
		return std::unexpected(Error{ErrorCode::InvalidArgument, "The owner already has a pending file dialog."});
	if (mode == Mode::Save && config.m_Multiple)
		return std::unexpected(Error{ErrorCode::InvalidArgument, "Save dialogs cannot select multiple files."});
	for (const auto& filter : config.m_Filters)
		if (filter.m_Name.empty() || filter.m_Pattern.empty() || filter.m_Name.find('\0') != std::string::npos || filter.m_Pattern.find('\0') != std::string::npos)
			return std::unexpected(Error{ErrorCode::InvalidArgument, "File filters require nonempty names and patterns without null characters."});
	void* handle = nullptr;
#if defined(_WIN32)
	handle = SDL_GetPointerProperty(SDL_GetWindowProperties(owner.m_Impl->m_Native), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif
	const auto nativeMode = mode == Mode::Save ? Detail::FileDialogMode::Save : mode == Mode::Folder ? Detail::FileDialogMode::Folder : Detail::FileDialogMode::Open;
	auto task = Detail::FileDialogTask::Start(owner.GetDispatcher(), Detail::NativeFileDialog(handle, nativeMode, std::move(config)), std::move(callback));
	if (!task) return std::unexpected(task.error());
	owner.m_Impl->m_FileDialog = std::move(*task);
	return {};
}
void FileDialog::Cancel(Window& owner) { if (owner.m_Impl->m_FileDialog) owner.m_Impl->m_FileDialog->Cancel(); }
}
