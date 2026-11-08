#include "IconValidation.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <cstring>
#include <cwchar>

namespace YukiriDemo
{
bool CheckWindowIcons()
{
	const auto module = GetModuleHandleW(nullptr);
	const auto group = FindResourceW(module, MAKEINTRESOURCEW(1), MAKEINTRESOURCEW(14));
	if (!group) return false;
	const auto* bytes = static_cast<const unsigned char*>(LockResource(LoadResource(module, group)));
	constexpr std::array Sizes{16, 20, 24, 32, 40, 48, 64, 128, 256};
	if (!bytes || SizeofResource(module, group) < 6 + Sizes.size() * 14 || bytes[4] != Sizes.size() || bytes[5]) return false;
	for (std::size_t i = 0; i < Sizes.size(); ++i)
	{
		const auto* entry = bytes + 6 + i * 14;
		if (entry[0] != Sizes[i] % 256 || entry[1] != entry[0]) return false;
		WORD identifier = 0;
		std::memcpy(&identifier, entry + 12, sizeof(identifier));
		if (!FindResourceW(module, MAKEINTRESOURCEW(identifier), MAKEINTRESOURCEW(3))) return false;
	}
	int matches = 0;
	EnumThreadWindows(GetCurrentThreadId(), [](HWND handle, LPARAM context) -> BOOL
	{
		wchar_t title[128]{};
		GetWindowTextW(handle, title, static_cast<int>(std::size(title)));
		if (std::wcscmp(title, L"Yukiri - Widget Gallery") != 0 && std::wcscmp(title, L"Yukiri - Native Title Bar") != 0) return TRUE;
		if (!GetClassLongPtrW(handle, GCLP_HICON)) return FALSE;
		++*reinterpret_cast<int*>(context);
		return TRUE;
	}, reinterpret_cast<LPARAM>(&matches));
	return matches == 2;
}
}
#else
namespace YukiriDemo
{
bool CheckWindowIcons() { return true; }
}
#endif
