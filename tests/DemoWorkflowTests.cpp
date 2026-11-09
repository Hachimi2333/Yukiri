#include "../demo/DocumentFiles.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <fstream>
#include <chrono>

namespace
{
class TemporaryDirectory
{
public:
	TemporaryDirectory()
	{
		static std::atomic<unsigned> sequence = 0;
		m_Path = std::filesystem::temp_directory_path() / ("Yukiri-document-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + std::to_string(++sequence));
		std::filesystem::create_directory(m_Path);
	}
	~TemporaryDirectory()
	{
		// Only remove the files created by this test, never recurse through an external path.
		std::error_code error;
		for (const auto& file : std::filesystem::directory_iterator(m_Path, error)) std::filesystem::remove(file.path(), error);
		std::filesystem::remove(m_Path, error);
	}
	std::filesystem::path m_Path;
};
void Raw(const std::filesystem::path& path, std::string_view text) { std::ofstream file(path, std::ios::binary); file.write(text.data(), static_cast<std::streamsize>(text.size())); }
}
TEST_CASE("Demo document snapshots preserve unsaved edits made while saving", "[demo][document]")
{
	YukiriDemo::DocumentSession document;
	REQUIRE_FALSE(document.IsDirty());
	document.SetText("First version / 第一版");
	REQUIRE(document.IsDirty());
	const auto snapshot = document.GetText();
	document.SetText("Second version / 第二版");
	document.MarkSaved("document.txt", snapshot);
	REQUIRE(document.IsDirty());
	REQUIRE(document.GetText() == "Second version / 第二版");
	document.SetText(snapshot);
	REQUIRE_FALSE(document.IsDirty());
	document.Load("other.txt", "Loaded / 已读取");
	REQUIRE_FALSE(document.IsDirty());
	document.New();
	REQUIRE(document.GetPath().empty());
	REQUIRE(document.GetText().empty());
	REQUIRE_FALSE(document.IsDirty());
}
TEST_CASE("Demo file IO handles UTF-8 paths, BOMs, newline normalization and replacement", "[demo][document]")
{
	TemporaryDirectory temporary;
	const auto path = temporary.m_Path / std::filesystem::path(u8"测试文档.txt");
	Raw(path, "\xEF\xBB\xBF" "Line one\r\n你好\rLast\n");
	auto loaded = YukiriDemo::ReadDocument(path); REQUIRE(loaded);
	REQUIRE(*loaded == "Line one\n你好\nLast\n");
	REQUIRE(YukiriDemo::WriteDocument(path, "Updated / 已更新\n"));
	loaded = YukiriDemo::ReadDocument(path); REQUIRE(loaded);
	REQUIRE(*loaded == "Updated / 已更新\n");
	REQUIRE(YukiriDemo::PathText(path.filename()) == "测试文档.txt");
	REQUIRE(std::distance(std::filesystem::directory_iterator(temporary.m_Path), std::filesystem::directory_iterator{}) == 1);
}
TEST_CASE("Demo document IO rejects invalid data and preserves the destination on cancellation", "[demo][document]")
{
	TemporaryDirectory temporary;
	const auto path = temporary.m_Path / "document.txt";
	REQUIRE(YukiriDemo::WriteDocument(path, "Keep me"));
	std::stop_source cancellation; cancellation.request_stop();
	REQUIRE_FALSE(YukiriDemo::WriteDocument(path, "Discard me", cancellation.get_token()));
	REQUIRE(YukiriDemo::ReadDocument(path).value() == "Keep me");
	REQUIRE_FALSE(YukiriDemo::ReadDocument(path, cancellation.get_token()));
	REQUIRE_FALSE(YukiriDemo::WriteDocument(path, std::string("A\0B", 3)));
	REQUIRE(YukiriDemo::ReadDocument(path).value() == "Keep me");
	for (const auto bytes : {std::string("\xC0\x80", 2), std::string("\xED\xA0\x80", 3), std::string("\xF4\x90\x80\x80", 4), std::string("\xFF\xFE", 2), std::string("\xE4\xBD", 2)})
	{
		Raw(path, bytes); REQUIRE_FALSE(YukiriDemo::ReadDocument(path));
	}
	REQUIRE_FALSE(YukiriDemo::ReadDocument(temporary.m_Path / "missing.txt"));
	REQUIRE_FALSE(YukiriDemo::WriteDocument(temporary.m_Path / "missing" / "file.txt", "test"));
	REQUIRE(std::distance(std::filesystem::directory_iterator(temporary.m_Path), std::filesystem::directory_iterator{}) == 1);
}
