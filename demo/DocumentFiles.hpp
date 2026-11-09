#pragma once

#include <Yukiri/Core/Error.hpp>
#include <filesystem>
#include <stop_token>
#include <string>
#include <string_view>

namespace YukiriDemo
{
// File operations belong to the demo, not the GUI framework.
[[nodiscard]] Yukiri::Result<std::string> ReadDocument(const std::filesystem::path& path, std::stop_token stop = {});
[[nodiscard]] Yukiri::Result<void> WriteDocument(const std::filesystem::path& path, std::string_view text, std::stop_token stop = {});
[[nodiscard]] std::string PathText(const std::filesystem::path& path);

class DocumentSession
{
public:
	void New();
	void Load(std::filesystem::path path, std::string text);
	void SetText(std::string text);
	void MarkSaved(std::filesystem::path path, std::string savedText);
	[[nodiscard]] bool IsDirty() const noexcept;
	[[nodiscard]] const std::string& GetText() const noexcept { return m_Text; }
	[[nodiscard]] const std::filesystem::path& GetPath() const noexcept { return m_Path; }
private:
	std::filesystem::path m_Path;
	std::string m_Text;
	std::string m_SavedText;
};
}
