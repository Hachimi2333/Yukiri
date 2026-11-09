#pragma once

#include <Yukiri/Core/Error.hpp>
#include <memory>

struct SDL_Window;
namespace Yukiri::Detail
{
class WidgetTree;
class WindowsAccessibility
{
public:
	static Result<std::unique_ptr<WindowsAccessibility>> Create(SDL_Window* window, WidgetTree& tree);
	~WindowsAccessibility();
	void Flush();
private:
	struct Impl;
	explicit WindowsAccessibility(std::unique_ptr<Impl> impl);
	std::unique_ptr<Impl> m_Impl;
};
}
