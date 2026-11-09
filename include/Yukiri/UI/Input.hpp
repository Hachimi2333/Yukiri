#pragma once

#include <Yukiri/Core/Geometry.hpp>
#include <string>

namespace Yukiri
{
enum class Key
{
	Unknown, Tab, Enter, Space, Escape, Left, Right, Up, Down, Home, End, Backspace, Delete,
	Insert, PageUp, PageDown, Menu,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
	Minus, Plus, Comma, Period, Slash, Backslash, Semicolon, Quote, LeftBracket, RightBracket, Backtick,
	Shift, Control, Alt, Super, CapsLock, NumLock, ScrollLock, PrintScreen, Pause
};
enum class PointerButton { None, Left, Middle, Right };
enum class Cursor { Arrow, Hand, Text };

struct Modifiers
{
	bool m_Shift = false;
	bool m_Control = false;
	bool m_Alt = false;
	bool m_Super = false;
	bool m_AltGraph = false;
	bool operator==(const Modifiers&) const = default;
};

struct PointerEvent
{
	Point m_Position;
	PointerButton m_Button = PointerButton::None;
	Modifiers m_Modifiers;
	float m_WheelX = 0;
	float m_WheelY = 0;
	bool m_Handled = false;
	int m_ClickCount = 1;
};

struct KeyEvent
{
	Key m_Key = Key::Unknown;
	Modifiers m_Modifiers;
	bool m_Repeat = false;
	bool m_Handled = false;
};
}
