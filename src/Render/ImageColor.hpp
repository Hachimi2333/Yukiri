#pragma once

#include <cstdint>
#include <span>

namespace Yukiri::Detail
{
void PrepareImageUpload(std::span<std::uint8_t> rgba);
}
