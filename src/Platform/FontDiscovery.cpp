#include "FontDiscovery.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwrite.h>
#include <memory>
#include <algorithm>

namespace Yukiri::Detail
{
namespace
{
struct ReleaseCom
{
	template<typename T> void operator()(T* value) const noexcept { if (value) value->Release(); }
};
template<typename T> using ComOwner = std::unique_ptr<T, ReleaseCom>;
}

std::vector<FontSource> FindFonts(const Yukiri::Font& request)
{
	std::vector<FontSource> sources;
	IDWriteFactory* rawFactory = nullptr;
	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&rawFactory)))) return sources;
	ComOwner<IDWriteFactory> factory(rawFactory);
	IDWriteFontCollection* rawCollection = nullptr;
	if (FAILED(factory->GetSystemFontCollection(&rawCollection))) return sources;
	ComOwner<IDWriteFontCollection> collection(rawCollection);
	const auto nameUtf8 = request.m_Family.empty() ? std::string("Segoe UI") : request.m_Family;
	if (nameUtf8.size() > 32768) return sources;
	const auto countUtf16 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, nameUtf8.data(), static_cast<int>(nameUtf8.size()), nullptr, 0);
	if (!countUtf16) return sources;
	std::wstring nameUtf16(static_cast<std::size_t>(countUtf16), L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, nameUtf8.data(), static_cast<int>(nameUtf8.size()), nameUtf16.data(), countUtf16);
	const wchar_t* families[] = {nameUtf16.c_str()};
	for (const auto* name : families)
	{
		UINT32 index = 0;
		BOOL exists = FALSE;
		if (FAILED(collection->FindFamilyName(name, &index, &exists)) || !exists) continue;
		IDWriteFontFamily* rawFamily = nullptr;
		if (FAILED(collection->GetFontFamily(index, &rawFamily))) continue;
		ComOwner<IDWriteFontFamily> family(rawFamily);
		IDWriteFont* rawFont = nullptr;
		if (FAILED(family->GetFirstMatchingFont(static_cast<DWRITE_FONT_WEIGHT>(std::clamp(request.m_Weight, 100, 900)),
			DWRITE_FONT_STRETCH_NORMAL, request.m_Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, &rawFont))) continue;
		ComOwner<IDWriteFont> font(rawFont);
		IDWriteFontFace* rawFace = nullptr;
		if (FAILED(font->CreateFontFace(&rawFace))) continue;
		ComOwner<IDWriteFontFace> face(rawFace);
		UINT32 count = 0;
		if (FAILED(face->GetFiles(&count, nullptr))) continue;
		std::vector<IDWriteFontFile*> files(count);
		if (FAILED(face->GetFiles(&count, files.data()))) continue;
		for (auto* rawFile : files)
		{
			ComOwner<IDWriteFontFile> file(rawFile);
			const void* key = nullptr;
			UINT32 keySize = 0;
			IDWriteFontFileLoader* rawLoader = nullptr;
			if (FAILED(file->GetReferenceKey(&key, &keySize)) || FAILED(file->GetLoader(&rawLoader))) continue;
			ComOwner<IDWriteFontFileLoader> loader(rawLoader);
			IDWriteLocalFontFileLoader* rawLocal = nullptr;
			if (FAILED(loader->QueryInterface(__uuidof(IDWriteLocalFontFileLoader), reinterpret_cast<void**>(&rawLocal)))) continue;
			ComOwner<IDWriteLocalFontFileLoader> local(rawLocal);
			UINT32 length = 0;
			if (FAILED(local->GetFilePathLengthFromKey(key, keySize, &length))) continue;
			std::wstring path(static_cast<std::size_t>(length) + 1, L'\0');
			if (FAILED(local->GetFilePathFromKey(key, keySize, path.data(), length + 1))) continue;
			path.resize(length);
			sources.push_back({std::filesystem::path(path), static_cast<long>(face->GetIndex()), nameUtf8});
		}
	}
	return sources;
}
std::vector<FontSource> DiscoverFonts()
{
	std::vector<FontSource> sources;
	for (const auto* family : {"Segoe UI Variable", "Segoe UI", "Microsoft YaHei UI", "Microsoft YaHei", "SimSun", "Segoe UI Symbol"})
	{
		auto found = FindFonts({family});
		sources.insert(sources.end(), std::make_move_iterator(found.begin()), std::make_move_iterator(found.end()));
	}
	return sources;
}
std::optional<FontSource> ResolveFont(const Yukiri::Font& font)
{
	auto sources = FindFonts(font);
	return sources.empty() ? std::nullopt : std::optional<FontSource>(std::move(sources.front()));
}
}
#else
namespace Yukiri::Detail
{
std::vector<FontSource> DiscoverFonts()
{
	std::vector<FontSource> result;
	for (const char* path : {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"})
		if (std::filesystem::exists(path)) result.push_back({path, 0});
	return result;
}
std::optional<FontSource> ResolveFont(const Yukiri::Font&) { return std::nullopt; }
}
#endif
