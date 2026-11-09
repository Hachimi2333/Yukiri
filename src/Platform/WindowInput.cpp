#include "WindowState.hpp"

namespace Yukiri
{
namespace
{
Modifiers GetModifiers(SDL_Keymod modifiers)
{
	return {(modifiers & SDL_KMOD_SHIFT) != 0, (modifiers & SDL_KMOD_CTRL) != 0, (modifiers & SDL_KMOD_ALT) != 0,
		(modifiers & SDL_KMOD_GUI) != 0, (modifiers & SDL_KMOD_MODE) != 0 ||
		((modifiers & SDL_KMOD_RALT) != 0 && (modifiers & SDL_KMOD_CTRL) != 0)};
}
Key Translate(SDL_Keycode key)
{
	if (key >= 'A' && key <= 'Z') return static_cast<Key>(static_cast<int>(Key::A) + static_cast<int>(key - 'A'));
	if (key >= SDLK_A && key <= SDLK_Z) return static_cast<Key>(static_cast<int>(Key::A) + static_cast<int>(key - SDLK_A));
	if (key >= SDLK_0 && key <= SDLK_9) return static_cast<Key>(static_cast<int>(Key::D0) + static_cast<int>(key - SDLK_0));
	if (key >= SDLK_KP_1 && key <= SDLK_KP_9) return static_cast<Key>(static_cast<int>(Key::D1) + static_cast<int>(key - SDLK_KP_1));
	if (key >= SDLK_F1 && key <= SDLK_F12) return static_cast<Key>(static_cast<int>(Key::F1) + static_cast<int>(key - SDLK_F1));
	if (key >= SDLK_F13 && key <= SDLK_F24) return static_cast<Key>(static_cast<int>(Key::F13) + static_cast<int>(key - SDLK_F13));
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
	case SDLK_INSERT: return Key::Insert;
	case SDLK_KP_0: return Key::D0;
	case SDLK_PAGEUP: return Key::PageUp;
	case SDLK_PAGEDOWN: return Key::PageDown;
	case SDLK_APPLICATION: return Key::Menu;
	case SDLK_MINUS: case SDLK_KP_MINUS: return Key::Minus;
	case SDLK_PLUS: case SDLK_EQUALS: case SDLK_KP_PLUS: return Key::Plus;
	case SDLK_COMMA: return Key::Comma;
	case SDLK_PERIOD: case SDLK_KP_PERIOD: return Key::Period;
	case SDLK_SLASH: case SDLK_KP_DIVIDE: return Key::Slash;
	case SDLK_BACKSLASH: return Key::Backslash;
	case SDLK_SEMICOLON: return Key::Semicolon;
	case SDLK_APOSTROPHE: return Key::Quote;
	case SDLK_LEFTBRACKET: return Key::LeftBracket;
	case SDLK_RIGHTBRACKET: return Key::RightBracket;
	case SDLK_GRAVE: return Key::Backtick;
	case SDLK_LSHIFT: case SDLK_RSHIFT: return Key::Shift;
	case SDLK_LCTRL: case SDLK_RCTRL: return Key::Control;
	case SDLK_LALT: case SDLK_RALT: return Key::Alt;
	case SDLK_LGUI: case SDLK_RGUI: return Key::Super;
	case SDLK_CAPSLOCK: return Key::CapsLock;
	case SDLK_NUMLOCKCLEAR: return Key::NumLock;
	case SDLK_SCROLLLOCK: return Key::ScrollLock;
	case SDLK_PRINTSCREEN: return Key::PrintScreen;
	case SDLK_PAUSE: return Key::Pause;
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
	case SDL_EVENT_WINDOW_FOCUS_GAINED: m_Tree.GainedFocus(); break;
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
		pointer.m_ClickCount = event.button.clicks;
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
	case SDL_EVENT_KEY_UP:
		m_Tree.KeyUp({Translate(event.key.key), GetModifiers(event.key.mod), false});
		break;
	case SDL_EVENT_DROP_BEGIN: m_DroppedFiles.clear(); break;
	case SDL_EVENT_DROP_FILE:
		if (event.drop.data)
		{
			const std::string_view utf8(event.drop.data);
			m_DroppedFiles.emplace_back(std::u8string(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size()));
		}
		break;
	case SDL_EVENT_DROP_COMPLETE:
		if (!m_DroppedFiles.empty()) m_Owner->m_FilesDropped.Emit(std::exchange(m_DroppedFiles, {}));
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
			if (event.type == SDL_EVENT_WINDOW_RESIZED) m_Owner->m_SizeChanged.Emit(m_Owner->GetSize());
			if (event.type == SDL_EVENT_WINDOW_MINIMIZED || event.type == SDL_EVENT_WINDOW_MAXIMIZED ||
				event.type == SDL_EVENT_WINDOW_RESTORED || event.type == SDL_EVENT_WINDOW_SHOWN || event.type == SDL_EVENT_WINDOW_HIDDEN)
				m_Owner->m_StateChanged.Emit();
		}
		break;
	}
}
}
