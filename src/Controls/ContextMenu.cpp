#include <Yukiri/Controls/ContextMenu.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/TextLayout.hpp"
#include <algorithm>
#include <cmath>

namespace Yukiri
{
namespace
{
std::string ShortcutText(const std::optional<KeyGesture>& gesture)
{
	if (!gesture) return {};
	std::string text;
	if (gesture->m_Modifiers.m_Control) text += "Ctrl+";
	if (gesture->m_Modifiers.m_Alt) text += "Alt+";
	if (gesture->m_Modifiers.m_Shift) text += "Shift+";
	if (gesture->m_Modifiers.m_Super) text += "Win+";
	const auto key = gesture->m_Key;
	if (key >= Key::A && key <= Key::Z) text += static_cast<char>('A' + static_cast<int>(key) - static_cast<int>(Key::A));
	else if (key >= Key::D0 && key <= Key::D9) text += static_cast<char>('0' + static_cast<int>(key) - static_cast<int>(Key::D0));
	else if (key >= Key::F1 && key <= Key::F24) text += "F" + std::to_string(static_cast<int>(key) - static_cast<int>(Key::F1) + 1);
	else switch (key)
	{
	case Key::Delete: text += "Del"; break;
	case Key::Insert: text += "Ins"; break;
	case Key::Enter: text += "Enter"; break;
	case Key::Space: text += "Space"; break;
	case Key::Escape: text += "Esc"; break;
	case Key::Tab: text += "Tab"; break;
	case Key::Backspace: text += "Backspace"; break;
	case Key::Left: text += "Left"; break;
	case Key::Right: text += "Right"; break;
	case Key::Up: text += "Up"; break;
	case Key::Down: text += "Down"; break;
	case Key::Home: text += "Home"; break;
	case Key::End: text += "End"; break;
	case Key::PageUp: text += "PgUp"; break;
	case Key::PageDown: text += "PgDn"; break;
	default: text += "Key"; break;
	}
	return text;
}
}
ContextMenu::Item ContextMenu::Item::Action(std::shared_ptr<Command> command) { return {{}, std::move(command), {}, false}; }
ContextMenu::Item ContextMenu::Item::Submenu(std::string text, std::vector<Item> children) { return {std::move(text), {}, std::move(children), false}; }
ContextMenu::Item ContextMenu::Item::Divider() { return {{}, {}, {}, true}; }
ContextMenu::ContextMenu(std::vector<Item> items) : m_Items(std::move(items))
{
	SetFocusable(true);
	SetCursor(Cursor::Arrow);
	m_RootLifetime = ObserveLifetime();
	for (const auto& item : m_Items) if (item.m_Command)
	{
		AddCommand(item.m_Command);
		Keep(item.m_Command->m_Changed.Connect([this] { InvalidateLayout(); }));
	}
	Move(1);
}
const std::vector<ContextMenu::Item>& ContextMenu::GetItems() const noexcept { return m_Items; }
std::optional<std::size_t> ContextMenu::GetCurrentItem() const noexcept { return m_Current; }
Result<ContextMenu*> ContextMenu::Show(Widget& owner, std::vector<Item> items, Point localPosition)
{
	if (!owner.GetWindow()) return std::unexpected(Error{ErrorCode::InvalidArgument, "The menu owner must belong to a window."});
	auto menu = std::make_unique<ContextMenu>(std::move(items));
	auto* observer = menu.get();
	auto shown = owner.GetWindow()->OpenPopup(&owner, std::move(menu), {localPosition.m_X, localPosition.m_Y, 0, 0}, false, false, true);
	if (!shown) return std::unexpected(shown.error());
	return observer;
}
void ContextMenu::Bind(Widget& owner, std::vector<Item> items)
{
	owner.Keep(owner.m_ContextMenuRequested.Connect([&owner, items = std::move(items)](Point point) { auto shown = Show(owner, items, point); (void)shown; }));
}
bool ContextMenu::Selectable(std::size_t index) const
{
	const auto& item = m_Items[index];
	return !item.m_Separator && (!item.m_Command || item.m_Command->IsEnabled()) && (item.m_Command || !item.m_Children.empty());
}
float ContextMenu::RowHeight(std::size_t index) const { return m_Items[index].m_Separator ? 9.0f : std::max(32.0f, GetFontSize() * 1.5f + 8); }
float ContextMenu::RowTop(std::size_t index) const
{
	float y = 4 - m_Scroll;
	for (std::size_t row = 0; row < index; ++row) y += RowHeight(row);
	return y;
}
std::optional<std::size_t> ContextMenu::HitRow(Point point) const
{
	const auto local = ToLocal(point);
	if (!GetLocalBounds().Contains(local)) return {};
	float y = 4 - m_Scroll;
	for (std::size_t index = 0; index < m_Items.size(); ++index)
	{
		const auto height = RowHeight(index);
		if (local.m_Y >= y && local.m_Y < y + height) return Selectable(index) ? std::optional(index) : std::nullopt;
		y += height;
	}
	return {};
}
void ContextMenu::Select(std::optional<std::size_t> index, bool keyboard)
{
	if (m_Current != index)
	{
		m_HoverTimer.Stop();
		if (m_Child && !m_ChildLifetime.expired()) m_Child->ClosePopup();
		m_Child = nullptr;
		m_Current = index;
	}
	if (keyboard && index && GetActualSize().m_Height > 0)
	{
		const auto top = RowTop(*index) + m_Scroll;
		if (top < m_Scroll + 4) m_Scroll = std::max(0.0f, top - 4);
		else if (top + RowHeight(*index) > m_Scroll + GetActualSize().m_Height - 4)
			m_Scroll = std::max(0.0f, top + RowHeight(*index) - GetActualSize().m_Height + 4);
	}
	InvalidatePaint();
}
void ContextMenu::Move(int direction)
{
	if (m_Items.empty()) return;
	auto index = m_Current ? static_cast<int>(*m_Current) : direction > 0 ? -1 : 0;
	for (std::size_t visited = 0; visited < m_Items.size(); ++visited)
	{
		index = (index + direction + static_cast<int>(m_Items.size())) % static_cast<int>(m_Items.size());
		if (Selectable(static_cast<std::size_t>(index))) { Select(static_cast<std::size_t>(index), true); return; }
	}
	Select({}, true);
}
void ContextMenu::Dismiss() { if (!m_RootLifetime.expired()) m_Root->ClosePopup(); }
void ContextMenu::InvokeCurrent()
{
	if (!m_Current || !Selectable(*m_Current)) return;
	if (!m_Items[*m_Current].m_Children.empty()) { OpenSubmenu(); return; }
	auto command = m_Items[*m_Current].m_Command;
	if (!command) return;
	Dismiss();
	command->Execute();
}
void ContextMenu::OpenSubmenu()
{
	if (!m_Current || m_Items[*m_Current].m_Children.empty()) return;
	if (m_Child && !m_ChildLifetime.expired() && m_Child->IsPopupOpen()) { m_Child->RequestFocus(); return; }
	auto child = std::make_unique<ContextMenu>(m_Items[*m_Current].m_Children);
	child->m_Root = m_Root;
	child->m_RootLifetime = m_RootLifetime;
	child->m_SwitchMenu = m_SwitchMenu;
	auto* observer = child.get();
	auto shown = ShowPopupContent(std::move(child), {0, RowTop(*m_Current), GetActualSize().m_Width, RowHeight(*m_Current)}, true);
	if (shown) { m_Child = observer; m_ChildLifetime = ObserveLifetime(*observer); }
}
Size ContextMenu::OnMeasure(Size available)
{
	float width = 160;
	m_ContentHeight = 8;
	for (std::size_t index = 0; index < m_Items.size(); ++index)
	{
		const auto& item = m_Items[index];
		const auto& text = item.m_Command ? item.m_Command->GetText() : item.m_Text;
		const auto shortcut = item.m_Command ? ShortcutText(item.m_Command->GetShortcut()) : std::string{};
		width = std::max(width, MeasureText(text).m_Width + MeasureText(shortcut).m_Width + 88);
		m_ContentHeight += RowHeight(index);
	}
	return {std::min(width, available.m_Width), std::min(m_ContentHeight, available.m_Height)};
}
void ContextMenu::OnArrange(Rect content) { m_Scroll = std::clamp(m_Scroll, 0.0f, std::max(0.0f, m_ContentHeight - content.m_Height)); }
Rect ContextMenu::GetFocusBounds() const noexcept
{
	if (!m_Current) return {};
	return {5, RowTop(*m_Current) + 2, std::max(0.0f, GetActualSize().m_Width - 10), RowHeight(*m_Current) - 4};
}
void ContextMenu::OnPaint(Canvas& canvas)
{
	const auto& theme = GetTheme();
	const auto bounds = GetLocalBounds();
	canvas.FillRectangle(bounds, theme.m_Surface, GetRadius());
	canvas.StrokeRectangle(bounds, theme.m_Border, 1, GetRadius());
	canvas.PushClip({4, 4, std::max(0.0f, bounds.m_Width - 8), std::max(0.0f, bounds.m_Height - 8)});
	for (std::size_t index = 0; index < m_Items.size(); ++index)
	{
		const float y = RowTop(index), height = RowHeight(index);
		if (y + height < 4 || y > bounds.Bottom() - 4) continue;
		const auto& item = m_Items[index];
		if (item.m_Separator) { canvas.DrawLine({12, y + 4}, {bounds.m_Width - 12, y + 4}, theme.m_Border); continue; }
		const bool enabled = Selectable(index);
		const auto color = enabled ? GetForeground() : theme.m_MutedText;
		if (m_Current == index) canvas.FillRectangle({4, y, bounds.m_Width - 8, height}, theme.m_Hover, 4);
		if (item.m_Command && item.m_Command->IsChecked())
		{
			canvas.DrawLine({12, y + height / 2}, {16, y + height / 2 + 4}, theme.m_Accent, 2);
			canvas.DrawLine({16, y + height / 2 + 4}, {23, y + height / 2 - 5}, theme.m_Accent, 2);
		}
		else if (item.m_Command && item.m_Command->GetIcon()) canvas.DrawImage(item.m_Command->GetIcon(), {10, y + (height - 18) / 2, 18, 18}, enabled ? 1.0f : 0.5f);
		const auto& text = item.m_Command ? item.m_Command->GetText() : item.m_Text;
		const auto shortcut = item.m_Command ? ShortcutText(item.m_Command->GetShortcut()) : std::string{};
		const auto sw = canvas.MeasureText(shortcut, GetFontSize()).m_Width;
		const auto fitted = Detail::FitText(text, std::max(0.0f, bounds.m_Width - sw - 80), [&](std::string_view value) { return canvas.MeasureText(value, GetFontSize()); });
		const float ty = y + (height - GetFontSize() * 1.35f) / 2;
		canvas.DrawText(fitted, {34, ty}, color, GetFontSize());
		canvas.DrawText(shortcut, {bounds.m_Width - 28 - sw, ty}, theme.m_MutedText, GetFontSize());
		if (!item.m_Children.empty())
		{
			canvas.DrawLine({bounds.m_Width - 17, y + height / 2 - 4}, {bounds.m_Width - 13, y + height / 2}, color);
			canvas.DrawLine({bounds.m_Width - 13, y + height / 2}, {bounds.m_Width - 17, y + height / 2 + 4}, color);
		}
	}
	canvas.PopClip();
	PaintFocus(canvas);
}
void ContextMenu::OnPointerDown(PointerEvent& event) { if (event.m_Button == PointerButton::Left) { m_Pressed = HitRow(event.m_Position); event.m_Handled = true; } }
void ContextMenu::OnPointerUp(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	const auto hit = HitRow(event.m_Position);
	if (hit && hit == m_Pressed) { Select(hit, false); InvokeCurrent(); }
	m_Pressed.reset();
	event.m_Handled = true;
}
void ContextMenu::OnPointerMove(PointerEvent& event)
{
	const auto hit = HitRow(event.m_Position);
	if (hit == m_Current) return;
	Select(hit, false);
	if (hit && !m_Items[*hit].m_Children.empty() && GetWindow())
	{
		auto timer = GetWindow()->GetDispatcher().CreateTimer(std::chrono::milliseconds(180), false, [this] { OpenSubmenu(); });
		if (timer) m_HoverTimer = std::move(*timer);
	}
}
void ContextMenu::OnWheel(PointerEvent& event)
{
	m_HoverTimer.Stop();
	if (m_Child && !m_ChildLifetime.expired()) m_Child->ClosePopup(true);
	m_Scroll = std::clamp(m_Scroll - event.m_WheelY * 40, 0.0f, std::max(0.0f, m_ContentHeight - GetActualSize().m_Height));
	InvalidatePaint();
	event.m_Handled = true;
}
void ContextMenu::OnKeyDown(KeyEvent& event)
{
	switch (event.m_Key)
	{
	case Key::Down: Move(1); break;
	case Key::Up: Move(-1); break;
	case Key::Home: m_Current.reset(); Move(1); break;
	case Key::End: m_Current.reset(); Move(-1); break;
	case Key::PageDown: for (int count = 0; count < 8; ++count) Move(1); break;
	case Key::PageUp: for (int count = 0; count < 8; ++count) Move(-1); break;
	case Key::Enter: case Key::Space: InvokeCurrent(); break;
	case Key::Right:
		if (m_Current && !m_Items[*m_Current].m_Children.empty()) OpenSubmenu();
		else if (m_SwitchMenu) m_SwitchMenu(1);
		break;
	case Key::Left: if (m_Root != this) ClosePopup(); else if (m_SwitchMenu) m_SwitchMenu(-1); break;
	case Key::Escape: ClosePopup(); break;
	case Key::Tab: Dismiss(); break;
	default:
		for (const auto& item : m_Items) if (item.m_Command && item.m_Command->IsEnabled() && item.m_Command->GetShortcut() && item.m_Command->GetShortcut()->Matches(event))
		{
			if (event.m_Repeat) return;
			auto command = item.m_Command;
			Dismiss();
			command->Execute();
			event.m_Handled = true;
			return;
		}
		return;
	}
	event.m_Handled = true;
}
}
