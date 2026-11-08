#pragma once

#include <Yukiri/Core/Geometry.hpp>
#include <string>

namespace Yukiri
{
enum class Key
{
	Unknown, Tab, Enter, Space, Escape, Left, Right, Up, Down, Home, End, Backspace, Delete,
	A, C, X, V, Z, Y
};
enum class PointerButton { None, Left, Middle, Right };
enum class Cursor { Arrow, Hand, Text };

struct Modifiers
{
	bool m_Shift = false;
	bool m_Control = false;
	bool m_Alt = false;
};

struct PointerEvent
{
	Point m_Position;
	PointerButton m_Button = PointerButton::None;
	Modifiers m_Modifiers;
	float m_WheelX = 0;
	float m_WheelY = 0;
	bool m_Handled = false;
};

struct KeyEvent
{
	Key m_Key = Key::Unknown;
	Modifiers m_Modifiers;
	bool m_Repeat = false;
	bool m_Handled = false;
};
}
