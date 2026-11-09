#include "DocumentFiles.hpp"
#include <array>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <limits>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace YukiriDemo
{
namespace
{
constexpr std::size_t MaximumDocumentBytes = 32 * 1024 * 1024;
Yukiri::Error Failure(std::string message) { return {Yukiri::ErrorCode::ResourceLoading, std::move(message)}; }
bool ValidUtf8(std::string_view text)
{
	for (std::size_t index = 0; index < text.size();)
	{
		const auto first = static_cast<unsigned char>(text[index++]);
		if (!first) return false;
		if (first < 0x80) continue;
		const unsigned count = first >= 0xC2 && first <= 0xDF ? 1u : first >= 0xE0 && first <= 0xEF ? 2u : first >= 0xF0 && first <= 0xF4 ? 3u : 0u;
		if (!count || count > text.size() - index) return false;
		std::uint32_t scalar = first & ((1u << (6u - count)) - 1u);
		for (unsigned part = 0; part < count; ++part)
		{
			const auto next = static_cast<unsigned char>(text[index++]);
			if ((next & 0xC0) != 0x80) return false;
			scalar = (scalar << 6) | (next & 0x3F);
		}
		if ((count == 1 && scalar < 0x80) || (count == 2 && scalar < 0x800) || (count == 3 && scalar < 0x10000) ||
			scalar > 0x10FFFF || (scalar >= 0xD800 && scalar <= 0xDFFF)) return false;
	}
	return true;
}
struct TemporaryFile
{
	std::filesystem::path m_Path;
	bool m_Owns = false;
	~TemporaryFile() { if (m_Owns) { std::error_code error; std::filesystem::remove(m_Path, error); } }
};
}
std::string PathText(const std::filesystem::path& path)
{
	const auto utf8 = path.u8string();
	return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}
Yukiri::Result<std::string> ReadDocument(const std::filesystem::path& path, std::stop_token stop)
{
	try
	{
		std::ifstream input(path, std::ios::binary);
		if (!input) return std::unexpected(Failure("Cannot open the document for reading."));
		std::string text;
		std::array<char, 65536> buffer{};
		while (input)
		{
			if (stop.stop_requested()) return std::unexpected(Failure("Document operation canceled."));
			input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
			const auto bytes = static_cast<std::size_t>(input.gcount());
			if (bytes > MaximumDocumentBytes - text.size()) return std::unexpected(Failure("Demo documents are limited to 32 MiB."));
			text.append(buffer.data(), bytes);
		}
		if (!input.eof()) return std::unexpected(Failure("Reading the document failed."));
		if (text.starts_with("\xEF\xBB\xBF")) text.erase(0, 3);
		if (!ValidUtf8(text)) return std::unexpected(Failure("Open a UTF-8 text file. Binary and UTF-16 documents are not supported."));
		std::string normalized;
		normalized.reserve(text.size());
		for (std::size_t index = 0; index < text.size(); ++index)
		{
			if (text[index] == '\r') { normalized.push_back('\n'); if (index + 1 < text.size() && text[index + 1] == '\n') ++index; }
			else normalized.push_back(text[index]);
		}
		return normalized;
	}
	catch (const std::exception& error) { return std::unexpected(Failure(error.what())); }
}
Yukiri::Result<void> WriteDocument(const std::filesystem::path& path, std::string_view text, std::stop_token stop)
{
	try
	{
		if (text.size() > MaximumDocumentBytes || !ValidUtf8(text)) return std::unexpected(Failure("Save requires valid UTF-8 text up to 32 MiB."));
		static std::atomic<unsigned long long> sequence = 0;
		auto temporary = path;
		temporary += ".yukiri-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + std::to_string(++sequence) + ".tmp";
		TemporaryFile cleanup{temporary};
#if defined(_WIN32)
		const HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return std::unexpected(Failure("Cannot create a temporary document beside the destination."));
		cleanup.m_Owns = true;
		struct FileHandle { HANDLE m_Handle; ~FileHandle() { if (m_Handle != INVALID_HANDLE_VALUE) CloseHandle(m_Handle); } } handle{file};
		for (std::size_t offset = 0; offset < text.size();)
		{
			if (stop.stop_requested()) return std::unexpected(Failure("Document operation canceled."));
			const auto count = static_cast<DWORD>(std::min<std::size_t>(65536, text.size() - offset));
			DWORD written = 0;
			if (!WriteFile(file, text.data() + offset, count, &written, nullptr) || written != count) return std::unexpected(Failure("Writing the document failed."));
			offset += written;
		}
		if (!FlushFileBuffers(file)) return std::unexpected(Failure("Flushing the document failed."));
		CloseHandle(handle.m_Handle); handle.m_Handle = INVALID_HANDLE_VALUE;
		if (stop.stop_requested()) return std::unexpected(Failure("Document operation canceled."));
		if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return std::unexpected(Failure("Replacing the destination failed; the original document was preserved."));
#else
		std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
		if (!output) return std::unexpected(Failure("Cannot create the temporary document."));
		cleanup.m_Owns = true;
		output.write(text.data(), static_cast<std::streamsize>(text.size()));
		output.close();
		if (!output) return std::unexpected(Failure("Writing the document failed."));
		if (stop.stop_requested()) return std::unexpected(Failure("Document operation canceled."));
		std::filesystem::rename(temporary, path);
#endif
		return {};
	}
	catch (const std::exception& error) { return std::unexpected(Failure(error.what())); }
}
void DocumentSession::New() { m_Path.clear(); m_Text.clear(); m_SavedText.clear(); }
void DocumentSession::Load(std::filesystem::path path, std::string text) { m_Path = std::move(path); m_Text = std::move(text); m_SavedText = m_Text; }
void DocumentSession::SetText(std::string text) { m_Text = std::move(text); }
void DocumentSession::MarkSaved(std::filesystem::path path, std::string savedText) { m_Path = std::move(path); m_SavedText = std::move(savedText); }
bool DocumentSession::IsDirty() const noexcept { return m_Text != m_SavedText; }
}
