#pragma once

#include <Yukiri/Core/Geometry.hpp>
#include <Yukiri/Styling/MotionSettings.hpp>
#include <optional>

namespace Yukiri
{
struct Theme
{
	Color m_Background;
	Color m_Surface;
	Color m_Hover;
	Color m_Pressed;
	Color m_Text;
	Color m_MutedText;
	Color m_Border;
	Color m_Accent;
	Color m_AccentHover;
	Color m_OnAccent;
	Color m_Selection;
	float m_FontSize = 14;
	float m_Radius = 6;
	float m_ControlHeight = 34;
	float m_Spacing = 8;
	MotionSettings m_Motion;
	[[nodiscard]] static Theme Light() noexcept;
	[[nodiscard]] static Theme Dark() noexcept;
};

struct WidgetStyle
{
	std::optional<Color> m_Background;
	std::optional<Color> m_Foreground;
	std::optional<Color> m_Border;
	std::optional<Color> m_Hover;
	std::optional<Color> m_Pressed;
	std::optional<float> m_Radius;
	std::optional<float> m_FontSize;
};
}
