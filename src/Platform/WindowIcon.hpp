#pragma once

#include <Yukiri/Core/Error.hpp>
#include <cstdint>
#include <optional>

struct SDL_Window;

namespace Yukiri::Detail
{
class RenderDevice;

[[nodiscard]] Result<std::optional<std::uint32_t>> GetWindowIcon(SDL_Window* window, RenderDevice& device);
}
