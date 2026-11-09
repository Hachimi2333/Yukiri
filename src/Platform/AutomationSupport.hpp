#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <UIAutomation.h>
#include <utility>
#include <string>
#include <span>
#include <vector>

namespace Yukiri::Detail
{
inline constexpr double ScrollNoChange = -1.0;
template<typename T> class ComPtr
{
public:
	ComPtr() = default;
	explicit ComPtr(T* pointer) : m_Pointer(pointer) {}
	~ComPtr() { if (m_Pointer) m_Pointer->Release(); }
	ComPtr(const ComPtr& other) : m_Pointer(other.m_Pointer) { if (m_Pointer) m_Pointer->AddRef(); }
	ComPtr& operator=(const ComPtr& other) { if (this != &other) { ComPtr copy(other); std::swap(m_Pointer, copy.m_Pointer); } return *this; }
	ComPtr(ComPtr&& other) noexcept : m_Pointer(std::exchange(other.m_Pointer, nullptr)) {}
	ComPtr& operator=(ComPtr&& other) noexcept { if (this != &other) { ComPtr move(std::move(other)); std::swap(m_Pointer, move.m_Pointer); } return *this; }
	T* Get() const noexcept { return m_Pointer; }
	T* operator->() const noexcept { return m_Pointer; }
	T** Put() { if (m_Pointer) m_Pointer->Release(); m_Pointer = nullptr; return &m_Pointer; }
	T* Detach() noexcept { return std::exchange(m_Pointer, nullptr); }
private:
	T* m_Pointer = nullptr;
};
std::wstring ToWide(std::string_view text);
std::string ToUtf8(std::wstring_view text);
HRESULT StringValue(std::string_view text, BSTR* result);
HRESULT UnknownArray(std::span<const ComPtr<IUnknown>> values, SAFEARRAY** result);
HRESULT DoubleArray(std::span<const double> values, SAFEARRAY** result);
}
