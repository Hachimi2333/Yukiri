#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Core/Dispatcher.hpp>
#include <Yukiri/Platform/Config.hpp>

#include <memory>
#include <cstdint>
#include <filesystem>
#ifdef CreateWindow
#undef CreateWindow
#endif

namespace Yukiri
{
class Window;
class Command;

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
	[[nodiscard]] Dispatcher GetDispatcher() const;
	void AddCommand(std::shared_ptr<Command> command);
	void RemoveCommand(const Command& command);
	[[nodiscard]] std::uint32_t GetValidationErrorCount() const noexcept;
	[[nodiscard]] Result<void> LoadFont(const std::filesystem::path& path, long faceIndex = 0);

private:
	struct Impl;
	explicit Application(std::unique_ptr<Impl> impl);
	std::unique_ptr<Impl> m_Impl;
};
}
