#pragma once

#include <Yukiri/Core/Error.hpp>
#include <functional>
#include <memory>

struct SDL_Window;

namespace Yukiri::Detail
{
class WindowEvents
{
public:
	static Result<std::unique_ptr<WindowEvents>> Create(SDL_Window* window, std::function<void()> captureLost);
	~WindowEvents();
private:
	struct Impl;
	explicit WindowEvents(std::unique_ptr<Impl> impl);
	std::unique_ptr<Impl> m_Impl;
};
}
