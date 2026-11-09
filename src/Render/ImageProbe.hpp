#pragma once
#include "ImageData.hpp"

namespace Yukiri::Detail
{
Result<void> ProbeRasterImage(std::span<const std::uint8_t> encoded);
}
