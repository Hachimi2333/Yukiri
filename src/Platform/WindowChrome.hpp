#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Core/Geometry.hpp>
#include <memory>

struct SDL_Window;
namespace Yukiri
{
class Window;
class Canvas;
namespace Detail
{
class WindowChrome
{
public:
	static Result<std::unique_ptr<WindowChrome>> Create(Window& owner, SDL_Window* native);
	~WindowChrome();
	void Paint(Canvas& canvas);
	[[nodiscard]] float GetHeight() const noexcept { return 40; }
private:
	struct Impl;
	explicit WindowChrome(std::unique_ptr<Impl> impl);
	std::unique_ptr<Impl> m_Impl;
};
}
}
