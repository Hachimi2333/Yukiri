#include "WindowState.hpp"

namespace Yukiri
{
namespace
{
Modifiers GetModifiers(SDL_Keymod modifiers)
{
	return {(modifiers & SDL_KMOD_SHIFT) != 0, (modifiers & SDL_KMOD_CTRL) != 0, (modifiers & SDL_KMOD_ALT) != 0};
}
Key Translate(SDL_Keycode key)
{
	switch (key)
	{
	case SDLK_TAB: return Key::Tab;
	case SDLK_RETURN: case SDLK_KP_ENTER: return Key::Enter;
	case SDLK_SPACE: return Key::Space;
	case SDLK_ESCAPE: return Key::Escape;
	case SDLK_LEFT: return Key::Left;
	case SDLK_RIGHT: return Key::Right;
	case SDLK_UP: return Key::Up;
	case SDLK_DOWN: return Key::Down;
	case SDLK_HOME: return Key::Home;
	case SDLK_END: return Key::End;
	case SDLK_BACKSPACE: return Key::Backspace;
	case SDLK_DELETE: return Key::Delete;
	case SDLK_A: return Key::A;
	case SDLK_C: return Key::C;
	case SDLK_X: return Key::X;
	case SDLK_V: return Key::V;
	case SDLK_Z: return Key::Z;
	case SDLK_Y: return Key::Y;
	default: return Key::Unknown;
	}
}
}

void Window::Impl::HandleEvent(const SDL_Event& event)
{
	const float scale = m_Owner->GetScale();
	switch (event.type)
	{
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED: m_Owner->Close(); break;
	case SDL_EVENT_WINDOW_FOCUS_LOST: m_Tree.LostFocus(); break;
	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		m_Tree.PointerMove({{-1, -1}, PointerButton::None, {}});
		break;
	case SDL_EVENT_MOUSE_MOTION:
		m_Tree.PointerMove({{event.motion.x / scale, event.motion.y / scale}, PointerButton::None, GetModifiers(SDL_GetModState())});
		{
			const auto* target = m_Tree.m_Capture ? m_Tree.m_Capture : m_Tree.m_Hover;
			const auto cursor = static_cast<std::size_t>(target ? target->GetCursor() : Cursor::Arrow);
			if (m_Cursors[cursor]) SDL_SetCursor(m_Cursors[cursor]);
		}
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
	{
		PointerEvent pointer{{event.button.x / scale, event.button.y / scale},
			event.button.button == SDL_BUTTON_LEFT ? PointerButton::Left :
			event.button.button == SDL_BUTTON_RIGHT ? PointerButton::Right : PointerButton::Middle, GetModifiers(SDL_GetModState())};
		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) m_Tree.PointerDown(pointer); else m_Tree.PointerUp(pointer);
		break;
	}
	case SDL_EVENT_MOUSE_WHEEL:
	{
		const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
		m_Tree.Wheel({{event.wheel.mouse_x / scale, event.wheel.mouse_y / scale}, PointerButton::None,
			GetModifiers(SDL_GetModState()), event.wheel.x * direction, event.wheel.y * direction});
		break;
	}
	case SDL_EVENT_KEY_DOWN:
		m_Tree.KeyDown({Translate(event.key.key), GetModifiers(event.key.mod), event.key.repeat});
		break;
	case SDL_EVENT_TEXT_INPUT: m_Tree.TextInput(event.text.text); break;
	case SDL_EVENT_TEXT_EDITING: m_Tree.TextEditing(event.edit.text, event.edit.start, event.edit.length); break;
	default:
		if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
		{
			if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
				event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) m_Tree.ClosePopup(true);
			if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED || event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED)
				SDL_SetWindowMinimumSize(m_Native, static_cast<int>(static_cast<float>(m_Config.m_MinWidth) * scale),
					static_cast<int>(static_cast<float>(m_Config.m_MinHeight) * scale));
			if (event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED) RefreshRate();
			if (event.type == SDL_EVENT_WINDOW_RESTORED || event.type == SDL_EVENT_WINDOW_SHOWN) m_Pacer.Reset();
			m_Tree.m_LayoutDirty = true;
			m_Owner->Invalidate();
		}
		break;
	}
}
}
