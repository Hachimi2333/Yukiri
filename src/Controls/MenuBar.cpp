#include <Yukiri/Controls/MenuBar.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <algorithm>

namespace Yukiri
{
namespace
{
std::string Caption(std::string_view text)
{
	std::string result;
	for (std::size_t index = 0; index < text.size(); ++index)
	{
		if (text[index] == '&' && index + 1 < text.size()) { if (text[index + 1] == '&') ++index; else continue; }
		result += text[index];
	}
	return result;
}
}
MenuBar::MenuBar() { SetFocusable(true); }
const std::vector<MenuBar::Item>& MenuBar::GetItems() const noexcept { return m_Items; }
void MenuBar::SetItems(std::vector<Item> items)
{
	ClosePopup(true);
	m_Menu = nullptr;
	m_Items = std::move(items);
	m_Current = 0;
	m_Bindings.clear();
	m_BoundWindow = nullptr;
	InvalidateLayout();
}
void MenuBar::BindCommands()
{
	if (!GetWindow() || m_BoundWindow == GetWindow()) return;
	m_Bindings.clear();
	m_BoundWindow = GetWindow();
	const auto lifetime = ObserveLifetime();
	auto bind = [&](KeyGesture gesture, std::function<void()> action)
	{
		auto command = std::make_shared<Command>();
		command->SetShortcut(gesture);
		m_Bindings.push_back(command->m_Executed.Connect([this, lifetime, action = std::move(action)]
		{
			if (!lifetime.expired() && IsVisible() && IsEnabled()) action();
		}));
		m_Bindings.push_back(GetWindow()->ConnectCommand(std::move(command)));
	};
	bind({Key::F10, {}}, [this] { RequestFocus(); Open(m_Current); });
	std::function<void(const std::vector<Item>&)> registerActions = [&](const auto& items)
	{
		for (const auto& item : items)
		{
			if (item.m_Command) m_Bindings.push_back(GetWindow()->ConnectCommand(item.m_Command));
			registerActions(item.m_Children);
		}
	};
	registerActions(m_Items);
	for (std::size_t index = 0; index < m_Items.size(); ++index)
	{
		const auto& text = m_Items[index].m_Text;
		for (std::size_t offset = 0; offset + 1 < text.size(); ++offset)
		{
			if (text[offset] != '&') continue;
			const char character = text[++offset];
			if (character == '&') continue;
			const char upper = character >= 'a' && character <= 'z' ? static_cast<char>(character - 'a' + 'A') : character;
			if (upper >= 'A' && upper <= 'Z') bind({static_cast<Key>(static_cast<int>(Key::A) + upper - 'A'), {false, false, true}}, [this, index] { RequestFocus(); Open(index); });
			break;
		}
	}
}
Size MenuBar::OnMeasure(Size)
{
	float width = 0;
	for (const auto& item : m_Items) width += MeasureText(Caption(item.m_Text)).m_Width + 24;
	return {width, std::max(GetTheme().m_ControlHeight, GetFontSize() * 1.5f + 8)};
}
void MenuBar::OnArrange(Rect content)
{
	BindCommands();
	m_Headers.clear();
	float x = 0;
	for (const auto& item : m_Items)
	{
		const float width = MeasureText(Caption(item.m_Text)).m_Width + 24;
		m_Headers.push_back({x, 0, width, content.m_Height});
		x += width;
	}
}
Rect MenuBar::GetFocusBounds() const noexcept
{
	if (m_Current >= m_Headers.size()) return {};
	const auto rect = m_Headers[m_Current];
	return {rect.m_X + 2, rect.m_Y + 2, std::max(0.0f, rect.m_Width - 4), std::max(0.0f, rect.m_Height - 4)};
}
void MenuBar::OnPaint(Canvas& canvas)
{
	canvas.FillRectangle(GetLocalBounds(), GetTheme().m_Surface);
	const bool opened = m_Menu && !m_MenuLifetime.expired() && IsPopupOpen();
	for (std::size_t index = 0; index < m_Headers.size(); ++index)
	{
		const auto rect = m_Headers[index];
		if ((HasFocus() || opened) && m_Current == index) canvas.FillRectangle({rect.m_X + 2, 2, rect.m_Width - 4, rect.m_Height - 4}, GetTheme().m_Hover, 4);
		canvas.DrawText(Caption(m_Items[index].m_Text), {rect.m_X + 12, (rect.m_Height - GetFontSize() * 1.35f) / 2}, GetForeground(), GetFontSize());
	}
	PaintFocus(canvas);
}
void MenuBar::Open(std::size_t index)
{
	if (index >= m_Items.size() || index >= m_Headers.size()) return;
	ClosePopup(true);
	m_Current = index;
	auto menu = std::make_unique<ContextMenu>(m_Items[index].m_Children);
	const auto lifetime = ObserveLifetime();
	menu->m_SwitchMenu = [this, lifetime](int direction) { if (!lifetime.expired()) { RequestFocus(); Move(direction); Open(m_Current); } };
	auto* observer = menu.get();
	auto shown = ShowPopupContent(std::move(menu), m_Headers[index]);
	if (shown) { m_Menu = observer; m_MenuLifetime = ObserveLifetime(*observer); }
	InvalidatePaint();
}
void MenuBar::Move(int direction)
{
	if (!m_Items.empty()) m_Current = static_cast<std::size_t>((static_cast<int>(m_Current) + direction + static_cast<int>(m_Items.size())) % static_cast<int>(m_Items.size()));
	InvalidatePaint();
}
void MenuBar::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	const auto point = ToLocal(event.m_Position);
	for (std::size_t index = 0; index < m_Headers.size(); ++index) if (m_Headers[index].Contains(point)) { Open(index); event.m_Handled = true; return; }
}
void MenuBar::OnPointerMove(PointerEvent& event)
{
	if (!m_Menu || m_MenuLifetime.expired() || !IsPopupOpen()) return;
	const auto point = ToLocal(event.m_Position);
	for (std::size_t index = 0; index < m_Headers.size(); ++index) if (index != m_Current && m_Headers[index].Contains(point)) { Open(index); return; }
}
void MenuBar::OnKeyDown(KeyEvent& event)
{
	switch (event.m_Key)
	{
	case Key::Right: Move(1); break;
	case Key::Left: Move(-1); break;
	case Key::Home: m_Current = 0; InvalidatePaint(); break;
	case Key::End: m_Current = m_Items.empty() ? 0 : m_Items.size() - 1; InvalidatePaint(); break;
	case Key::Down: case Key::Enter: case Key::Space: Open(m_Current); break;
	case Key::Escape: ClosePopup(); break;
	default: return;
	}
	event.m_Handled = true;
}
}
