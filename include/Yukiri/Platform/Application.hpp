#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Platform/Config.hpp>

#include <memory>

namespace Yukiri
{
class Window;

class Application final
{
public:
	[[nodiscard]] static Result<std::unique_ptr<Application>> Create(const ApplicationConfig& config = {});
	~Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	[[nodiscard]] Result<Window*> CreateWindow(const WindowConfig& config = {});
	[[nodiscard]] Result<void> Run();
	[[nodiscard]] Result<bool> Poll(bool wait = true);
	void Quit();

private:
	struct Impl;
	explicit Application(std::unique_ptr<Impl> impl);
	std::unique_ptr<Impl> m_Impl;
};
}
