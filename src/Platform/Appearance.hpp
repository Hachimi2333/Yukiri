#pragma once

#include <SDL3/SDL.h>

namespace Yukiri::Detail
{
bool IsSystemDark();
void ApplyAppearance(SDL_Window* window, bool dark);
}
