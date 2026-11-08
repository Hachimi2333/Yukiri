#pragma once

#include <expected>
#include <string>

namespace Yukiri
{
enum class ErrorCode
{
	InvalidArgument,
	PlatformInitialization,
	WindowCreation,
	VulkanUnavailable,
	UnsupportedDevice,
	GraphicsInitialization,
	GraphicsOperation,
	FontLoading,
	ResourceLoading
};

struct Error
{
	ErrorCode m_Code = ErrorCode::InvalidArgument;
	std::string m_Message;
};

template<typename T>
using Result = std::expected<T, Error>;
}
