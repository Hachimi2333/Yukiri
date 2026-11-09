#include "AutomationTextRange.hpp"

namespace Yukiri::Detail
{
namespace
{
int Unit(TextUnit unit)
{
	switch (unit) { case TextUnit_Character: return 0; case TextUnit_Word: return 1; case TextUnit_Line: return 2; case TextUnit_Paragraph: return 3; case TextUnit_Page: return 4; case TextUnit_Format: case TextUnit_Document: return 5; default: return -1; }
}
std::size_t Enclosing(const std::vector<std::size_t>& boundaries, std::size_t offset)
{
	if (boundaries.size() < 2) return 0;
	const auto after = std::ranges::upper_bound(boundaries, offset);
	return std::min(static_cast<std::size_t>(after - boundaries.begin() - 1), boundaries.size() - 2);
}
}
HRESULT AutomationTextRange::ExpandToEnclosingUnit(TextUnit unit)
{
	if (Unit(unit) < 0) return E_INVALIDARG;
	return Read([&]
	{
		const auto boundaries = m_Context->m_Tree->TextBoundaries(m_Node, Unit(unit));
		if (boundaries.size() < 2) { m_Anchor->m_Start = m_Anchor->m_End = 0; return S_OK; }
		const auto index = Enclosing(boundaries, m_Anchor->m_Start);
		m_Anchor->m_Start = boundaries[index]; m_Anchor->m_End = boundaries[index + 1];
		return S_OK;
	});
}
HRESULT AutomationTextRange::Move(TextUnit unit, int count, int* result)
{
	if (!result) return E_POINTER;
	*result = 0;
	if (Unit(unit) < 0) return E_INVALIDARG;
	return Read([&]() -> HRESULT
	{
		const auto boundaries = m_Context->m_Tree->TextBoundaries(m_Node, Unit(unit));
		if (boundaries.size() < 2) return S_OK;
		if (m_Anchor->m_Start == m_Anchor->m_End)
		{
			int moved = 0;
			const auto status = MoveEndpointByUnit(TextPatternRangeEndpoint_Start, unit, count, &moved);
			m_Anchor->m_End = m_Anchor->m_Start; *result = moved; return status;
		}
		const auto index = Enclosing(boundaries, m_Anchor->m_Start);
		const auto target = static_cast<std::size_t>(std::clamp(static_cast<std::int64_t>(index) + count, std::int64_t(0), static_cast<std::int64_t>(boundaries.size()) - 2));
		*result = static_cast<int>(static_cast<std::int64_t>(target) - static_cast<std::int64_t>(index));
		m_Anchor->m_Start = boundaries[target]; m_Anchor->m_End = boundaries[target + 1];
		return S_OK;
	});
}
HRESULT AutomationTextRange::MoveEndpointByUnit(TextPatternRangeEndpoint endpoint, TextUnit unit, int count, int* result)
{
	if (!result) return E_POINTER;
	*result = 0;
	if (Unit(unit) < 0 || (endpoint != TextPatternRangeEndpoint_Start && endpoint != TextPatternRangeEndpoint_End)) return E_INVALIDARG;
	return Read([&]
	{
		if (count == 0) return S_OK;
		const auto boundaries = m_Context->m_Tree->TextBoundaries(m_Node, Unit(unit));
		const auto offset = endpoint == TextPatternRangeEndpoint_Start ? m_Anchor->m_Start : m_Anchor->m_End;
		std::size_t target = offset;
		if (count > 0)
		{
			const auto next = std::ranges::upper_bound(boundaries, offset);
			const auto available = static_cast<std::int64_t>(boundaries.end() - next);
			const auto moved = std::min(static_cast<std::int64_t>(count), available);
			if (moved) target = *(next + moved - 1);
			*result = static_cast<int>(moved);
		}
		else
		{
			const auto next = std::ranges::lower_bound(boundaries, offset);
			const auto available = static_cast<std::int64_t>(next - boundaries.begin());
			const auto moved = std::min(-static_cast<std::int64_t>(count), available);
			if (moved) target = *(next - moved);
			*result = -static_cast<int>(moved);
		}
		if (endpoint == TextPatternRangeEndpoint_Start) { m_Anchor->m_Start = target; m_Anchor->m_End = std::max(m_Anchor->m_End, target); }
		else { m_Anchor->m_End = target; m_Anchor->m_Start = std::min(m_Anchor->m_Start, target); }
		return S_OK;
	});
}
}
