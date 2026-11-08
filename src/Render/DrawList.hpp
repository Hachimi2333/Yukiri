#pragma once

#include <Yukiri/Core/Geometry.hpp>
#include <Yukiri/Core/Error.hpp>
#include <cstdint>
#include <vector>
#include <cmath>
#include <optional>

namespace Yukiri::Detail
{
struct Vertex
{
	Point m_Position;
	Point m_Local;
	Size m_Size;
	Color m_Color;
	float m_Radius = 0;
	float m_Border = 0;
	float m_Kind = 0;
	float m_Reserved = 0;
	Point m_Uv;
};

struct DrawBatch
{
	Rect m_Clip;
	std::uint32_t m_Texture = 0;
	std::uint32_t m_First = 0;
	std::uint32_t m_Count = 0;
};

class DrawList
{
public:
	void Add(Rect rectangle, Color color, Rect clip, float radius = 0, float border = 0,
		std::uint32_t texture = 0, Rect uv = {0, 0, 1, 1}, bool glyph = false, float rotation = 0)
	{
		if (rectangle.Empty() || clip.Empty() || color.m_A <= 0)
			return;
		const auto first = static_cast<std::uint32_t>(m_Vertices.size());
		const Point corners[] = {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}};
		for (const auto corner : corners)
		{
			const float localX = (corner.m_X - 0.5f) * rectangle.m_Width;
			const float localY = (corner.m_Y - 0.5f) * rectangle.m_Height;
			m_Vertices.push_back({{rectangle.m_X + rectangle.m_Width * 0.5f + localX * std::cos(rotation) - localY * std::sin(rotation),
				rectangle.m_Y + rectangle.m_Height * 0.5f + localX * std::sin(rotation) + localY * std::cos(rotation)},
				{corner.m_X * rectangle.m_Width, corner.m_Y * rectangle.m_Height},
				{rectangle.m_Width, rectangle.m_Height}, color, radius, border,
				glyph ? 1.0f : 0.0f, 0,
				{uv.m_X + corner.m_X * uv.m_Width, uv.m_Y + corner.m_Y * uv.m_Height}});
		}
		if (!m_Batches.empty())
		{
			auto& last = m_Batches.back();
			if (last.m_Texture == texture && last.m_Clip.m_X == clip.m_X && last.m_Clip.m_Y == clip.m_Y &&
				last.m_Clip.m_Width == clip.m_Width && last.m_Clip.m_Height == clip.m_Height)
			{
				last.m_Count += 6;
				return;
			}
		}
		m_Batches.push_back({clip, texture, first, 6});
	}

	std::vector<Vertex> m_Vertices;
	std::vector<DrawBatch> m_Batches;
	std::optional<Error> m_Error;
};
}
