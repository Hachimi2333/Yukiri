#pragma once

#include <Yukiri/Styling/MotionSettings.hpp>
#include <string>

namespace Yukiri
{
enum class WindowDecoration { Native, Custom };
enum class ThemeMode { System, Light, Dark };

struct ApplicationConfig
{
	std::string m_Name = "Yukiri Application";
	bool m_EnableValidation = true;
	MotionSettings m_Motion{};
};

struct WindowConfig
{
	std::string m_Title = "Yukiri";
	int m_Width = 1100;
	int m_Height = 760;
	int m_MinWidth = 330;
	int m_MinHeight = 240;
	bool m_Visible = true;
	WindowDecoration m_Decoration = WindowDecoration::Custom;
	ThemeMode m_Theme = ThemeMode::System;
};
}
