#pragma once

#include <Yukiri/Core/Error.hpp>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Yukiri
{
class Window;
class FileDialog final
{
public:
	struct Filter
	{
		std::string m_Name;
		std::string m_Pattern;
	};
	struct Config
	{
		std::string m_Title;
		std::filesystem::path m_DefaultPath;
		std::vector<Filter> m_Filters;
		std::string m_DefaultExtension;
		bool m_Multiple = false;
	};
	using Callback = std::function<void(Result<std::vector<std::filesystem::path>>)>;
	[[nodiscard]] static Result<void> Open(Window& owner, Config config, Callback callback);
	[[nodiscard]] static Result<void> Save(Window& owner, Config config, Callback callback);
	[[nodiscard]] static Result<void> SelectFolder(Window& owner, Config config, Callback callback);
	static void Cancel(Window& owner);
private:
	enum class Mode { Open, Save, Folder };
	static Result<void> Show(Window& owner, Mode mode, Config config, Callback callback);
};
}
