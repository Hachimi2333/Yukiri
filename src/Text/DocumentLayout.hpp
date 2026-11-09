#pragma once

#include "TextLayout.hpp"
#include <memory>
#include <cstdint>

namespace Yukiri::Detail
{
class DocumentLayout
{
public:
	struct Line
	{
		std::size_t m_Start;
		std::size_t m_End;
		float m_Width;
		std::shared_ptr<const std::vector<std::pair<std::size_t, float>>> m_Carets;
	};
	void Update(std::string_view text, float width, bool wrap, float lineHeight, std::uint64_t generation,
		const TextMeasure& measure, const TextCarets& carets = {});
	[[nodiscard]] const std::vector<Line>& GetLines() const noexcept;
	[[nodiscard]] Size GetSize() const noexcept;
	[[nodiscard]] std::size_t LineAt(std::size_t offset, bool upstream = false) const;
	[[nodiscard]] float CaretX(std::size_t line, std::size_t offset) const;
	[[nodiscard]] std::size_t Hit(std::size_t line, float x) const;
	[[nodiscard]] std::pair<std::size_t, std::size_t> Visible(float top, float height) const;
	[[nodiscard]] float GetLineHeight() const noexcept;
private:
	struct Paragraph
	{
		std::string m_Text;
		std::vector<Line> m_Lines;
	};
	std::vector<std::unique_ptr<Paragraph>> m_Paragraphs;
	std::vector<Line> m_Lines;
	float m_Width = -1;
	float m_LineHeight = 1;
	bool m_Wrap = false;
	std::uint64_t m_Generation = 0;
	Size m_Size;
};
}
