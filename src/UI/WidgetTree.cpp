#include "WidgetTree.hpp"
#include <Yukiri/Render/Canvas.hpp>
#include <algorithm>
#include <cassert>

namespace Yukiri::Detail
{
WidgetTree::Dispatch::Dispatch(WidgetTree& tree) : m_Tree(tree) { ++m_Tree.m_DispatchDepth; }
WidgetTree::Dispatch::~Dispatch() { if (--m_Tree.m_DispatchDepth == 0) m_Tree.Flush(); }
void WidgetTree::Invoke(std::function<void()> action)
{
	Dispatch dispatch(*this);
	action();
}
bool WidgetTree::Within(const Widget* candidate, const Widget* ancestor)
{
	for (auto* current = candidate; current; current = current->GetParent())
		if (current == ancestor) return true;
	return false;
}
WidgetTree::~WidgetTree()
{
	m_Lifetime.reset();
	m_Destroying = true;
	m_ToolTipTimer.Stop();
	m_Focus = m_Capture = m_Hover = nullptr;
	m_LastFocus = nullptr;
	auto popups = std::move(m_Popups);
	popups.clear();
	m_Root.reset();
}
void WidgetTree::SetRoot(std::unique_ptr<Widget> root)
{
	auto transfer = std::make_shared<std::unique_ptr<Widget>>(std::move(root));
	Defer([this, transfer]
	{
		m_Root = std::move(*transfer);
		++m_AccessibilityStructureVersion;
		if (m_Root) { assert(!m_Root->m_Parent); m_Root->Attach(this); }
		m_LayoutDirty = m_Dirty = true;
	});
}
void WidgetTree::Layout(Size size, Point origin)
{
	Dispatch dispatch(*this);
	if (!m_LayoutDirty && size.m_Width == m_LastSize.m_Width && size.m_Height == m_LastSize.m_Height &&
		origin.m_X == m_LastOrigin.m_X && origin.m_Y == m_LastOrigin.m_Y) return;
	m_LastSize = size;
	m_LastOrigin = origin;
	m_LayoutDirty = false;
	if (m_Root)
	{
		m_Root->Measure(size);
		m_Root->Arrange({origin.m_X, origin.m_Y, size.m_Width, size.m_Height});
	}
	LayoutPopups(size, origin);
}
void WidgetTree::Paint(Canvas& canvas)
{
	Dispatch dispatch(*this);
	if (m_Root) m_Root->Render(canvas);
	PaintPopups(canvas);
}
void WidgetTree::Defer(std::function<void()> action)
{
	m_Deferred.push_back(std::move(action));
	if (!m_DispatchDepth) Flush();
}
void WidgetTree::Flush()
{
	if (m_Flushing) return;
	m_Flushing = true;
	struct FlushScope
	{
		bool& m_Flushing;
		~FlushScope() { m_Flushing = false; }
	} scope{m_Flushing};
	Dispatch dispatch(*this);
	while (!m_Deferred.empty())
	{
		auto deferred = std::exchange(m_Deferred, {});
		for (auto& action : deferred) action();
	}
}
void WidgetTree::SetFocus(Widget* widget, bool bringIntoView)
{
	Dispatch dispatch(*this);
	if (widget && widget->m_Tree != this) return;
	if (widget && (!widget->IsEnabled() || !widget->IsVisible() || !widget->IsFocusable())) return;
	if (auto* modal = GetModalRoot(); widget && modal && !Within(widget, modal)) return;
	for (const auto& popup : m_Popups)
		if (widget && popup->m_Closing && popup->m_Content && Within(widget, popup->m_Content.get())) return;
	for (auto* ancestor = widget; ancestor; ancestor = ancestor->GetParent()) if (!ancestor->IsVisible()) return;
	if (m_Focus == widget)
	{
		if (m_Focus && bringIntoView && m_FocusVisible) m_Focus->BringFocusIntoView();
		return;
	}
	auto* previous = std::exchange(m_Focus, widget);
	if (previous) previous->OnFocusChanged(false);
	if (m_Focus) { m_Focus->OnFocusChanged(true); if (m_Focus && bringIntoView) m_Focus->BringFocusIntoView(); }
	m_Dirty = true;
}
void WidgetTree::SetFocusVisible(bool visible)
{
	if (m_FocusVisible == visible) return;
	m_FocusVisible = visible;
	m_Dirty = true;
}
void WidgetTree::Forget(Widget* widget)
{
	if (m_Destroying) return;
	Dispatch dispatch(*this);
	if (Within(m_ToolTipOwner, widget)) HideToolTip();
	if (Within(m_LastFocus, widget)) m_LastFocus = nullptr;
	if (Within(m_Focus, widget))
	{
		auto* previous = std::exchange(m_Focus, nullptr);
		previous->OnFocusChanged(false);
		if (m_EndTextInput) m_EndTextInput();
	}
	if (Within(m_Capture, widget))
	{
		CancelPointer();
	}
	if (Within(m_Hover, widget)) { m_Hover->SetHovered(false); m_Hover = nullptr; }
	for (std::size_t index = 0; index < m_Popups.size(); ++index)
		if (Within(m_Popups[index]->m_Owner, widget) || m_Popups[index]->m_Content.get() == widget) CloseFrom(index, true);
	m_Dirty = true;
}
Widget* WidgetTree::Target(Point position)
{
	return m_Capture ? m_Capture : HitTarget(position);
}
Widget* WidgetTree::HitTarget(Point position)
{
	for (auto entry = m_Popups.rbegin(); entry != m_Popups.rend(); ++entry)
	{
		const auto& popup = **entry;
		if (popup.m_Closing || !popup.m_Interactive || !popup.m_Content) continue;
		if (VisibleBounds(popup).Contains(position))
			if (auto* target = popup.m_Content->HitTest(position)) return target;
		if (popup.m_Modal) return nullptr;
	}
	return m_Root ? m_Root->HitTest(position) : nullptr;
}
void WidgetTree::Route(PointerEvent& event, void (Widget::*handler)(PointerEvent&))
{
	for (auto* widget = Target(event.m_Position); widget && !event.m_Handled; widget = widget->GetParent())
		(widget->*handler)(event);
}
void WidgetTree::PointerDown(PointerEvent event)
{
	Dispatch dispatch(*this);
	m_PointerPosition = event.m_Position;
	HideToolTip();
	SetFocusVisible(false);
	if (PopupPointer(event, true)) return;
	auto* focus = Target(event.m_Position);
	if (!focus && GetModalRoot()) return;
	while (focus && !focus->IsFocusable()) focus = focus->GetParent();
	SetFocus(focus, false);
	Route(event, &Widget::OnPointerDown);
}
void WidgetTree::PointerUp(PointerEvent event)
{
	Dispatch dispatch(*this);
	Route(event, &Widget::OnPointerUp);
	if (!event.m_Handled && event.m_Button == PointerButton::Right) Route(event, &Widget::OnContextMenu);
}
void WidgetTree::PointerMove(PointerEvent event)
{
	Dispatch dispatch(*this);
	m_PointerPosition = event.m_Position;
	if (PopupPointer(event, false)) return;
	auto* hover = HitTarget(event.m_Position);
	if (hover != m_Hover)
	{
		if (m_Hover) { m_Hover->SetHovered(false); m_Hover->OnPointerLeave(); }
		m_Hover = hover;
		if (m_Hover) { m_Hover->SetHovered(true); m_Hover->OnPointerEnter(); }
		m_Dirty = true;
		UpdateToolTip();
	}
	Route(event, &Widget::OnPointerMove);
}
void WidgetTree::Wheel(PointerEvent event)
{
	Dispatch dispatch(*this);
	HideToolTip();
	if (PopupPointer(event, false)) return;
	if (!ParentPopup(Target(event.m_Position))) ClosePopup(true);
	Route(event, &Widget::OnWheel);
}
void WidgetTree::AdvanceFocus(bool backwards)
{
	std::vector<Widget*> candidates;
	std::function<void(Widget&)> visit = [&](Widget& widget)
	{
		if (!widget.IsVisible() || !widget.IsEnabled()) return;
		if (widget.IsFocusable() && widget.IsTabStop()) candidates.push_back(&widget);
		for (const auto& child : widget.GetChildren()) visit(*child);
	};
	if (auto* root = FocusRoot()) visit(*root);
	if (candidates.empty()) { SetFocus(nullptr); return; }
	auto found = std::ranges::find(candidates, m_Focus);
	const auto count = static_cast<int>(candidates.size());
	int index = found == candidates.end() ? (backwards ? 0 : -1) : static_cast<int>(found - candidates.begin());
	index = (index + (backwards ? -1 : 1) + count) % count;
	SetFocusVisible(true);
	SetFocus(candidates[static_cast<std::size_t>(index)]);
}
void WidgetTree::KeyDown(KeyEvent event)
{
	Dispatch dispatch(*this);
	if (m_Focus && m_Focus->IsComposing()) { m_Focus->OnKeyDown(event); return; }
	if (PopupKey(event)) return;
	std::vector<Widget*> path;
	for (auto* widget = m_Focus; widget; widget = widget->GetParent()) path.push_back(widget);
	for (auto widget = path.rbegin(); widget != path.rend() && !event.m_Handled; ++widget) (*widget)->OnPreviewKeyDown(event);
	if (!path.empty() && m_Focus != path.front()) return;
	for (auto* widget = m_Focus; widget && !event.m_Handled; widget = widget->GetParent()) widget->OnKeyDown(event);
	if (!path.empty() && m_Focus != path.front()) return;
	if (!event.m_Handled && (event.m_Key == Key::Menu || (event.m_Key == Key::F10 && event.m_Modifiers.m_Shift)))
	{
		if (m_Focus)
		{
			const auto bounds = m_Focus->GetBounds();
			PointerEvent request{{bounds.m_X, bounds.Bottom()}, PointerButton::None, {}};
			for (auto* widget = m_Focus; widget && !request.m_Handled; widget = widget->GetParent()) widget->OnContextMenu(request);
			event.m_Handled = request.m_Handled;
		}
	}
	for (auto* widget = m_Focus; widget && !event.m_Handled; widget = widget->GetParent())
	{
		const auto commands = widget->m_Commands;
		for (const auto& command : commands) if (command->TryExecute(event)) break;
	}
	if (!event.m_Handled && !GetModalRoot())
	{
		const auto commands = m_Commands;
		for (const auto& command : commands) if (command->TryExecute(event)) break;
	}
	if (!event.m_Handled && !GetModalRoot()) m_CommandBindings.Emit(event);
	if (!event.m_Handled && !GetModalRoot() && m_ApplicationCommand) m_ApplicationCommand(event);
	if (!event.m_Handled && event.m_Key == Key::Tab && !event.m_Modifiers.m_Control && !event.m_Modifiers.m_Alt && !event.m_Modifiers.m_Super)
	{
		ClosePopup();
		AdvanceFocus(event.m_Modifiers.m_Shift);
	}
}
void WidgetTree::KeyUp(KeyEvent event)
{
	Dispatch dispatch(*this);
	for (auto* widget = m_Focus; widget && !event.m_Handled; widget = widget->GetParent()) widget->OnKeyUp(event);
}
void WidgetTree::CancelPointer()
{
	SetCapture(nullptr);
}
void WidgetTree::SetCapture(Widget* widget)
{
	Dispatch dispatch(*this);
	if (m_Capture == widget) return;
	if (widget && !widget->IsEnabled()) return;
	if (auto* modal = GetModalRoot(); widget && modal && !Within(widget, modal)) return;
	for (auto* ancestor = widget; ancestor; ancestor = ancestor->GetParent()) if (!ancestor->IsVisible()) return;
	auto* previous = std::exchange(m_Capture, widget);
	if (previous) previous->OnPointerCaptureLost();
	if (m_PointerCapture) m_PointerCapture(m_Capture != nullptr);
}
void WidgetTree::TextInput(std::string_view text) { Dispatch dispatch(*this); if (m_Focus) m_Focus->OnTextInput(text); }
void WidgetTree::TextEditing(std::string_view text, int start, int length)
{
	Dispatch dispatch(*this);
	if (m_Focus) m_Focus->OnTextEditing(text, start, length);
}
void WidgetTree::LostFocus()
{
	Dispatch dispatch(*this);
	HideToolTip();
	SetFocusVisible(false);
	ClosePopup(true);
	m_LastFocus = m_Focus;
	SetFocus(nullptr);
	CancelPointer();
}
void WidgetTree::GainedFocus()
{
	Dispatch dispatch(*this);
	if (!m_Focus && m_LastFocus) SetFocus(m_LastFocus, false);
	if (!m_Focus) if (auto* root = FocusRoot()) FocusFirst(*root);
}
void WidgetTree::Tick(std::chrono::steady_clock::time_point now)
{
	Dispatch dispatch(*this);
	m_Animating = m_Root && m_Root->Tick(now);
	TickPopups(now);
}
}
