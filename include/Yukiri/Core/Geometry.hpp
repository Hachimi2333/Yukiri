#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Yukiri
{
struct Point
{
	float m_X = 0;
	float m_Y = 0;
	Point operator+(Point other) const noexcept { return {m_X + other.m_X, m_Y + other.m_Y}; }
	Point operator-(Point other) const noexcept { return {m_X - other.m_X, m_Y - other.m_Y}; }
};

struct Size
{
	float m_Width = 0;
	float m_Height = 0;
};

struct Rect
{
	float m_X = 0;
	float m_Y = 0;
	float m_Width = 0;
	float m_Height = 0;
	[[nodiscard]] float Right() const noexcept { return m_X + m_Width; }
	[[nodiscard]] float Bottom() const noexcept { return m_Y + m_Height; }
	[[nodiscard]] bool Contains(Point point) const noexcept
	{
		return point.m_X >= m_X && point.m_Y >= m_Y && point.m_X < Right() && point.m_Y < Bottom();
	}
	[[nodiscard]] bool Empty() const noexcept { return m_Width <= 0 || m_Height <= 0; }
	[[nodiscard]] Rect Intersect(Rect other) const noexcept
	{
		const auto x = std::max(m_X, other.m_X);
		const auto y = std::max(m_Y, other.m_Y);
		return {x, y, std::max(0.0f, std::min(Right(), other.Right()) - x),
			std::max(0.0f, std::min(Bottom(), other.Bottom()) - y)};
	}
};

struct Insets
{
	float m_Left = 0;
	float m_Top = 0;
	float m_Right = 0;
	float m_Bottom = 0;
	Insets() = default;
	explicit Insets(float value) : m_Left(value), m_Top(value), m_Right(value), m_Bottom(value) {}
	Insets(float left, float top, float right, float bottom)
		: m_Left(left), m_Top(top), m_Right(right), m_Bottom(bottom) {}
	[[nodiscard]] float Horizontal() const noexcept { return m_Left + m_Right; }
	[[nodiscard]] float Vertical() const noexcept { return m_Top + m_Bottom; }
};

struct Color
{
	float m_R = 0;
	float m_G = 0;
	float m_B = 0;
	float m_A = 1;
	[[nodiscard]] static constexpr Color FromHex(std::uint32_t rgb, float alpha = 1) noexcept
	{
		return {static_cast<float>((rgb >> 16) & 255) / 255, static_cast<float>((rgb >> 8) & 255) / 255,
			static_cast<float>(rgb & 255) / 255, alpha};
	}
	[[nodiscard]] Color WithAlpha(float alpha) const noexcept { return {m_R, m_G, m_B, alpha}; }
};
}
