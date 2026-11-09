#include "WindowIcon.hpp"
#include "Render/ImageColor.hpp"
#include "Render/RenderDevice.hpp"
#include <algorithm>
#include <cstring>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace Yukiri::Detail
{
namespace
{
struct IconBitmap
{
	HICON m_Icon = nullptr;
	HDC m_Dc = nullptr;
	HBITMAP m_Bitmap = nullptr;
	HGDIOBJ m_Previous = nullptr;
	~IconBitmap()
	{
		if (m_Previous && m_Previous != HGDI_ERROR) SelectObject(m_Dc, m_Previous);
		if (m_Bitmap) DeleteObject(m_Bitmap);
		if (m_Dc) DeleteDC(m_Dc);
		if (m_Icon) DestroyIcon(m_Icon);
	}
};

Result<std::vector<unsigned char>> Rasterize(HICON source, int pixels)
{
	IconBitmap bitmap;
	bitmap.m_Icon = static_cast<HICON>(CopyImage(source, IMAGE_ICON, pixels, pixels, LR_COPYFROMRESOURCE));
	bitmap.m_Dc = CreateCompatibleDC(nullptr);
	BITMAPINFO info{};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = pixels;
	info.bmiHeader.biHeight = -pixels;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void* storage = nullptr;
	bitmap.m_Bitmap = CreateDIBSection(bitmap.m_Dc, &info, DIB_RGB_COLORS, &storage, nullptr, 0);
	if (!bitmap.m_Icon || !bitmap.m_Dc || !bitmap.m_Bitmap || !storage)
		return std::unexpected(Error{ErrorCode::PlatformOperation, "Creating the application icon bitmap failed."});
	bitmap.m_Previous = SelectObject(bitmap.m_Dc, bitmap.m_Bitmap);
	const auto bytes = static_cast<std::size_t>(pixels) * pixels * 4;
	std::memset(storage, 0, bytes);
	if (!bitmap.m_Previous || bitmap.m_Previous == HGDI_ERROR ||
		!DrawIconEx(bitmap.m_Dc, 0, 0, bitmap.m_Icon, pixels, pixels, 0, nullptr, DI_NORMAL))
		return std::unexpected(Error{ErrorCode::PlatformOperation, "Drawing the application icon bitmap failed."});
	GdiFlush();
	const auto* bgra = static_cast<const unsigned char*>(storage);
	std::vector<unsigned char> rgba(bytes);
	bool hasAlpha = false;
	for (std::size_t i = 0; i < bytes; i += 4) hasAlpha |= bgra[i + 3] != 0;
	for (std::size_t i = 0; i < bytes; i += 4)
	{
		const unsigned int alpha = hasAlpha ? bgra[i + 3] : 255;
		// GDI draws premultiplied BGRA; the sRGB texture uses straight RGBA.
		for (std::size_t channel = 0; channel < 3; ++channel)
			rgba[i + channel] = alpha ? static_cast<unsigned char>(std::min(255u,
				(static_cast<unsigned int>(bgra[i + 2 - channel]) * 255 + alpha / 2) / alpha)) : 0;
		rgba[i + 3] = static_cast<unsigned char>(alpha);
	}
	if (!hasAlpha)
	{
		// Legacy icons describe transparency through their AND mask instead.
		if (!DrawIconEx(bitmap.m_Dc, 0, 0, bitmap.m_Icon, pixels, pixels, 0, nullptr, DI_MASK))
			return std::unexpected(Error{ErrorCode::PlatformOperation, "Drawing the application icon mask failed."});
		GdiFlush();
		for (std::size_t i = 0; i < bytes; i += 4) rgba[i + 3] = bgra[i] ? 0 : 255;
	}
	return rgba;
}
}

Result<std::optional<std::uint32_t>> GetWindowIcon(SDL_Window* window, RenderDevice& device)
{
	const auto handle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window),
		SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
	if (!handle) return std::unexpected(Error{ErrorCode::PlatformOperation, "The window has no Windows handle."});
	// SDL's window class inherits the host executable's first icon resource.
	const auto icon = reinterpret_cast<HICON>(GetClassLongPtrW(handle, GCLP_HICON));
	if (!icon) return std::optional<std::uint32_t>{};
	const int pixels = std::clamp(MulDiv(16, static_cast<int>(GetDpiForWindow(handle)), 96), 8, 256);
	const auto key = std::make_pair(reinterpret_cast<std::uintptr_t>(icon), pixels);
	if (const auto found = device.m_ImageCache.find(key); found != device.m_ImageCache.end())
		return std::optional<std::uint32_t>{found->second};
	auto rgba = Rasterize(icon, pixels);
	if (!rgba) return std::unexpected(rgba.error());
	auto created = device.CreateTexture(static_cast<std::uint32_t>(pixels), static_cast<std::uint32_t>(pixels), true);
	if (!created) return std::unexpected(created.error());
	device.m_Textures[*created]->m_Pixels = std::move(*rgba);
	PrepareImageUpload(device.m_Textures[*created]->m_Pixels);
	device.m_ImageCache.emplace(key, *created);
	return std::optional<std::uint32_t>{*created};
}
}
#else
namespace Yukiri::Detail
{
Result<std::optional<std::uint32_t>> GetWindowIcon(SDL_Window*, RenderDevice&)
{
	return std::optional<std::uint32_t>{};
}
}
#endif
