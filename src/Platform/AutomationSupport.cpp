#include "AutomationSupport.hpp"
#include "Text/Unicode.hpp"
#include <limits>
#include <stdexcept>
#include <algorithm>

namespace Yukiri::Detail
{
std::wstring ToWide(std::string_view text)
{
	if (text.empty()) return {};
	const auto value = SanitizeUtf8(text);
	if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("Accessibility text is too large.");
	const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
	std::wstring result(static_cast<std::size_t>(size), L'\0');
	if (!size || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), size)) throw std::invalid_argument("Invalid accessibility UTF-8.");
	return result;
}
std::string ToUtf8(std::wstring_view text)
{
	if (text.empty()) return {};
	if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("Accessibility text is too large.");
	const auto size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
	std::string result(static_cast<std::size_t>(size), '\0');
	if (!size || !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr)) throw std::invalid_argument("Invalid accessibility UTF-16.");
	return result;
}
HRESULT StringValue(std::string_view text, BSTR* result)
{
	if (!result) return E_POINTER;
	const auto wide = ToWide(text);
	*result = SysAllocStringLen(wide.data(), static_cast<UINT>(wide.size()));
	return *result ? S_OK : E_OUTOFMEMORY;
}
HRESULT UnknownArray(std::span<const ComPtr<IUnknown>> values, SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	if (values.size() > std::numeric_limits<ULONG>::max()) return E_OUTOFMEMORY;
	const auto array = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(values.size()));
	if (!array) return E_OUTOFMEMORY;
	IUnknown** slots = nullptr;
	const auto accessed = SafeArrayAccessData(array, reinterpret_cast<void**>(&slots));
	if (FAILED(accessed)) { SafeArrayDestroy(array); return accessed; }
	for (std::size_t index = 0; index < values.size(); ++index) { slots[index] = values[index].Get(); if (slots[index]) slots[index]->AddRef(); }
	SafeArrayUnaccessData(array);
	*result = array;
	return S_OK;
}
HRESULT DoubleArray(std::span<const double> values, SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = SafeArrayCreateVector(VT_R8, 0, static_cast<ULONG>(values.size()));
	if (!*result) return E_OUTOFMEMORY;
	double* slots = nullptr;
	const auto accessed = SafeArrayAccessData(*result, reinterpret_cast<void**>(&slots));
	if (FAILED(accessed)) { SafeArrayDestroy(*result); *result = nullptr; return accessed; }
	std::copy(values.begin(), values.end(), slots);
	SafeArrayUnaccessData(*result);
	return S_OK;
}
}
