#pragma once

#include <Yukiri/Core/Error.hpp>
#include <memory>

namespace Yukiri::Detail
{
class UiApartment
{
public:
	static Result<std::unique_ptr<UiApartment>> Create();
	~UiApartment();
private:
	UiApartment() = default;
	bool m_Initialized = false;
};
}
