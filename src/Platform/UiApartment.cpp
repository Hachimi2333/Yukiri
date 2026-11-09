#include "UiApartment.hpp"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#endif

namespace Yukiri::Detail
{
Result<std::unique_ptr<UiApartment>> UiApartment::Create()
{
	auto apartment = std::unique_ptr<UiApartment>(new UiApartment());
#if defined(_WIN32)
	const auto result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(result)) return std::unexpected(Error{ErrorCode::PlatformInitialization, "The Yukiri UI thread requires a COM single-threaded apartment."});
	apartment->m_Initialized = true;
#endif
	return apartment;
}
UiApartment::~UiApartment()
{
#if defined(_WIN32)
	if (m_Initialized) CoUninitialize();
#endif
}
}
