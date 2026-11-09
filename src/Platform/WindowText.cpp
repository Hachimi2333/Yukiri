#include "WindowState.hpp"
#include <cmath>
#include <limits>

namespace Yukiri
{
std::uint64_t Window::GetFontGeneration() const noexcept { return m_Impl->m_Text->GetFontGeneration(); }
std::vector<std::pair<std::size_t, float>> Window::GetTextCarets(std::string_view text, float fontSize, const Font& font) const
{
	return m_Impl->m_Text->Carets(text, fontSize, GetScale(), font);
}
Size Window::MeasureText(std::string_view text, float fontSize, const Font& font) const { return m_Impl->m_Text->Measure(text, fontSize, GetScale(), font); }
float Window::GetTextCaretX(std::string_view text, std::size_t offset, float fontSize, const Font& font) const
{
	const auto carets = m_Impl->m_Text->Carets(text, fontSize, GetScale(), font);
	float position = 0;
	for (const auto& [boundary, x] : carets)
	{
		if (boundary > offset) break;
		position = x;
	}
	return position;
}
std::size_t Window::HitTestText(std::string_view text, float x, float fontSize, const Font& font) const
{
	const auto carets = m_Impl->m_Text->Carets(text, fontSize, GetScale(), font);
	std::size_t closest = 0;
	float distance = std::numeric_limits<float>::infinity();
	for (const auto& [boundary, position] : carets)
	{
		const float current = std::abs(position - x);
		if (current < distance) { closest = boundary; distance = current; }
	}
	return closest;
}
}
