#include "FontDiscovery.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwrite.h>
#include <memory>

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

std::vector<FontSource> DiscoverFonts()
{
	std::vector<FontSource> sources;
	IDWriteFactory* rawFactory = nullptr;
	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&rawFactory)))) return sources;
	ComOwner<IDWriteFactory> factory(rawFactory);
	IDWriteFontCollection* rawCollection = nullptr;
	if (FAILED(factory->GetSystemFontCollection(&rawCollection))) return sources;
	ComOwner<IDWriteFontCollection> collection(rawCollection);
	const wchar_t* families[] = {L"Segoe UI Variable", L"Segoe UI", L"Microsoft YaHei UI", L"Microsoft YaHei", L"SimSun", L"Segoe UI Symbol"};
	for (const auto* name : families)
	{
		UINT32 index = 0;
		BOOL exists = FALSE;
		if (FAILED(collection->FindFamilyName(name, &index, &exists)) || !exists) continue;
		IDWriteFontFamily* rawFamily = nullptr;
		if (FAILED(collection->GetFontFamily(index, &rawFamily))) continue;
		ComOwner<IDWriteFontFamily> family(rawFamily);
		IDWriteFont* rawFont = nullptr;
		if (FAILED(family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &rawFont))) continue;
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
			sources.push_back({std::filesystem::path(path), static_cast<long>(face->GetIndex())});
		}
	}
	return sources;
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
}
#endif
