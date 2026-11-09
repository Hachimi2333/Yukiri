#include "FileDialogTask.hpp"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>
#include <limits>
#include <stdexcept>

namespace Yukiri::Detail
{
namespace
{
template<typename T> struct ComOwner
{
	T* m_Pointer = nullptr;
	~ComOwner() { if (m_Pointer) m_Pointer->Release(); }
	T* operator->() const { return m_Pointer; }
};
std::wstring Wide(std::string_view text)
{
	if (text.empty()) return {};
	if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) || text.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid file dialog text.");
	const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
	if (!length) throw std::invalid_argument("File dialog strings must be valid UTF-8.");
	std::wstring result(static_cast<std::size_t>(length), L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), length);
	return result;
}
Error Failure(HRESULT result) { return {ErrorCode::PlatformOperation, "Windows file dialog failed (HRESULT " + std::to_string(static_cast<long>(result)) + ")."}; }
Result<std::filesystem::path> Path(IShellItem* item)
{
	PWSTR text = nullptr;
	const auto result = item->GetDisplayName(SIGDN_FILESYSPATH, &text);
	if (FAILED(result)) return std::unexpected(Failure(result));
	std::filesystem::path path(text);
	CoTaskMemFree(text);
	return path;
}
struct DialogHost
{
	IFileDialog* m_Dialog = nullptr;
	static LRESULT CALLBACK Procedure(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
	{
		if (message == WM_NCCREATE)
			SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams));
		if (message == WM_APP + 42)
		{
			auto* host = reinterpret_cast<DialogHost*>(GetWindowLongPtrW(handle, GWLP_USERDATA));
			if (host && host->m_Dialog) host->m_Dialog->Close(HRESULT_FROM_WIN32(ERROR_CANCELLED));
			return 0;
		}
		return DefWindowProcW(handle, message, wparam, lparam);
	}
};
Result<std::vector<std::filesystem::path>> Run(HWND owner, FileDialogMode mode, const FileDialog::Config& config, DialogCancellation& cancellation)
{
	if (cancellation.IsCanceled()) return std::vector<std::filesystem::path>{};
	const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(initialized)) return std::unexpected(Failure(initialized));
	struct ComScope { ~ComScope() { CoUninitialize(); } } scope;
	ComOwner<IFileDialog> dialog;
	auto result = CoCreateInstance(mode == FileDialogMode::Save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr,
		CLSCTX_INPROC_SERVER, IID_IFileDialog, reinterpret_cast<void**>(&dialog.m_Pointer));
	if (FAILED(result)) return std::unexpected(Failure(result));
	FILEOPENDIALOGOPTIONS options{};
	result = dialog->GetOptions(&options);
	if (FAILED(result)) return std::unexpected(Failure(result));
	options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
	if (mode == FileDialogMode::Folder) options |= FOS_PICKFOLDERS;
	else if (mode == FileDialogMode::Open) options |= FOS_FILEMUSTEXIST;
	else options |= FOS_OVERWRITEPROMPT;
	if (config.m_Multiple && mode != FileDialogMode::Save) options |= FOS_ALLOWMULTISELECT;
	result = dialog->SetOptions(options);
	if (FAILED(result)) return std::unexpected(Failure(result));
	const auto title = Wide(config.m_Title), extension = Wide(config.m_DefaultExtension);
	if (!title.empty() && FAILED(result = dialog->SetTitle(title.c_str()))) return std::unexpected(Failure(result));
	if (!extension.empty() && FAILED(result = dialog->SetDefaultExtension(extension.c_str()))) return std::unexpected(Failure(result));
	std::vector<std::pair<std::wstring, std::wstring>> ownedFilters;
	for (const auto& filter : config.m_Filters) ownedFilters.emplace_back(Wide(filter.m_Name), Wide(filter.m_Pattern));
	std::vector<COMDLG_FILTERSPEC> filters;
	for (const auto& [name, pattern] : ownedFilters) filters.push_back({name.c_str(), pattern.c_str()});
	if (!filters.empty() && mode != FileDialogMode::Folder && FAILED(result = dialog->SetFileTypes(static_cast<UINT>(filters.size()), filters.data()))) return std::unexpected(Failure(result));
	if (!config.m_DefaultPath.empty())
	{
		std::error_code error;
		const bool directory = std::filesystem::is_directory(config.m_DefaultPath, error);
		const auto folder = directory ? config.m_DefaultPath : config.m_DefaultPath.parent_path();
		if (!folder.empty())
		{
			ComOwner<IShellItem> item;
			result = SHCreateItemFromParsingName(folder.c_str(), nullptr, IID_IShellItem, reinterpret_cast<void**>(&item.m_Pointer));
			if (FAILED(result) || FAILED(result = dialog->SetFolder(item.m_Pointer))) return std::unexpected(Failure(result));
		}
		if (!directory && FAILED(result = dialog->SetFileName(config.m_DefaultPath.filename().c_str()))) return std::unexpected(Failure(result));
	}
	WNDCLASSW descriptor{};
	descriptor.lpfnWndProc = DialogHost::Procedure;
	descriptor.hInstance = GetModuleHandleW(nullptr);
	descriptor.lpszClassName = L"Yukiri.FileDialog.Cancellation";
	if (!RegisterClassW(&descriptor) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		return std::unexpected(Failure(HRESULT_FROM_WIN32(GetLastError())));
	DialogHost host{dialog.m_Pointer};
	const auto handle = CreateWindowExW(0, descriptor.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, descriptor.hInstance, &host);
	if (!handle) return std::unexpected(Failure(HRESULT_FROM_WIN32(GetLastError())));
	struct HostScope
	{
		DialogCancellation& m_Cancellation;
		HWND m_Handle;
		~HostScope() { m_Cancellation.Clear(); DestroyWindow(m_Handle); }
	} hostScope{cancellation, handle};
	cancellation.Install([handle] { PostMessageW(handle, WM_APP + 42, 0, 0); });
	if (cancellation.IsCanceled()) return std::vector<std::filesystem::path>{};
	result = dialog->Show(owner);
	if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED) || cancellation.IsCanceled()) return std::vector<std::filesystem::path>{};
	if (FAILED(result)) return std::unexpected(Failure(result));
	std::vector<std::filesystem::path> paths;
	if (mode == FileDialogMode::Save)
	{
		ComOwner<IShellItem> item;
		if (FAILED(result = dialog->GetResult(&item.m_Pointer))) return std::unexpected(Failure(result));
		auto path = Path(item.m_Pointer);
		if (!path) return std::unexpected(path.error());
		paths.push_back(std::move(*path));
	}
	else
	{
		ComOwner<IFileOpenDialog> open;
		if (FAILED(result = dialog->QueryInterface(IID_IFileOpenDialog, reinterpret_cast<void**>(&open.m_Pointer)))) return std::unexpected(Failure(result));
		ComOwner<IShellItemArray> items;
		if (FAILED(result = open->GetResults(&items.m_Pointer))) return std::unexpected(Failure(result));
		DWORD count = 0;
		if (FAILED(result = items->GetCount(&count))) return std::unexpected(Failure(result));
		for (DWORD index = 0; index < count; ++index)
		{
			ComOwner<IShellItem> item;
			if (FAILED(result = items->GetItemAt(index, &item.m_Pointer))) return std::unexpected(Failure(result));
			auto path = Path(item.m_Pointer);
			if (!path) return std::unexpected(path.error());
			paths.push_back(std::move(*path));
		}
	}
	return paths;
}
}
FileDialogTask::Backend NativeFileDialog(void* owner, FileDialogMode mode, FileDialog::Config config)
{
	return [owner, mode, config = std::move(config)](DialogCancellation& cancellation) { return Run(static_cast<HWND>(owner), mode, config, cancellation); };
}
}
#else
namespace Yukiri::Detail
{
FileDialogTask::Backend NativeFileDialog(void*, FileDialogMode, FileDialog::Config)
{
	return [](DialogCancellation&) -> Result<std::vector<std::filesystem::path>> { return std::unexpected(Error{ErrorCode::PlatformOperation, "File dialogs are not implemented on this platform."}); };
}
}
#endif
