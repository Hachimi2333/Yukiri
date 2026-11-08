#include "WidgetTree.hpp"
#include <Yukiri/Render/Canvas.hpp>
#include <algorithm>
#include <cassert>

namespace Yukiri::Detail
{
class WidgetTree::Dispatch
{
public:
	explicit Dispatch(WidgetTree& tree) : m_Tree(tree) { ++m_Tree.m_DispatchDepth; }
	~Dispatch() { if (--m_Tree.m_DispatchDepth == 0) m_Tree.Flush(); }
private:
	WidgetTree& m_Tree;
};

namespace
{
bool Within(Widget* candidate, Widget* ancestor)
{
	for (auto* current = candidate; current; current = current->GetParent())
		if (current == ancestor) return true;
	return false;
}
}

WidgetTree::~WidgetTree()
{
	m_Focus = m_Capture = m_Hover = nullptr;
	ClosePopup(true);
	m_Root.reset();
}
void WidgetTree::SetRoot(std::unique_ptr<Widget> root)
{
	auto transfer = std::make_shared<std::unique_ptr<Widget>>(std::move(root));
	Defer([this, transfer]
	{
		m_Root = std::move(*transfer);
		if (m_Root) { assert(!m_Root->m_Parent); m_Root->Attach(this); }
		m_LayoutDirty = m_Dirty = true;
	});
}
void WidgetTree::Layout(Size size, Point origin)
{
	Dispatch dispatch(*this);
	if (!m_Root || (!m_LayoutDirty && size.m_Width == m_LastSize.m_Width && size.m_Height == m_LastSize.m_Height &&
		origin.m_X == m_LastOrigin.m_X && origin.m_Y == m_LastOrigin.m_Y)) return;
	m_LastSize = size;
	m_LastOrigin = origin;
	m_LayoutDirty = false;
	m_Root->Measure(size);
	m_Root->Arrange({origin.m_X, origin.m_Y, size.m_Width, size.m_Height});
}
void WidgetTree::Paint(Canvas& canvas)
{
	Dispatch dispatch(*this);
	if (m_Root) m_Root->Render(canvas);
	if (m_PopupPaint && !m_PopupLifetime.expired())
	{
		auto paint = m_PopupPaint;
		canvas.Save();
		canvas.SetOffset({0, 0});
		paint(canvas, m_PopupProgress.GetValue());
		canvas.Restore();
	}
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
	if (widget && (!widget->IsEnabled() || !widget->IsVisible() || !widget->IsFocusable())) return;
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
	Dispatch dispatch(*this);
	if (Within(m_Focus, widget))
	{
		auto* previous = std::exchange(m_Focus, nullptr);
		previous->OnFocusChanged(false);
		if (m_EndTextInput) m_EndTextInput();
	}
	if (Within(m_Capture, widget))
	{
		m_Capture = nullptr;
		if (m_PointerCapture) m_PointerCapture(false);
	}
	if (Within(m_Hover, widget)) { m_Hover->SetHovered(false); m_Hover = nullptr; }
	if (Within(m_PopupOwner, widget)) ClosePopup(true);
	m_Dirty = true;
}
Widget* WidgetTree::Target(Point position) { return m_Capture ? m_Capture : m_Root ? m_Root->HitTest(position) : nullptr; }
void WidgetTree::Route(PointerEvent& event, void (Widget::*handler)(PointerEvent&))
{
	for (auto* widget = Target(event.m_Position); widget && !event.m_Handled; widget = widget->GetParent())
		(widget->*handler)(event);
}
void WidgetTree::PointerDown(PointerEvent event)
{
	Dispatch dispatch(*this);
	SetFocusVisible(false);
	if (m_PopupPointer && !m_PopupClosing)
	{
		auto pointer = m_PopupPointer;
		if (pointer(event)) return;
		ClosePopup();
	}
	auto* focus = Target(event.m_Position);
	while (focus && !focus->IsFocusable()) focus = focus->GetParent();
	SetFocus(focus, false);
	Route(event, &Widget::OnPointerDown);
}
void WidgetTree::PointerUp(PointerEvent event) { Dispatch dispatch(*this); Route(event, &Widget::OnPointerUp); }
void WidgetTree::PointerMove(PointerEvent event)
{
	Dispatch dispatch(*this);
	if (m_PopupPointer && !m_PopupClosing)
	{
		auto pointer = m_PopupPointer;
		if (pointer(event)) return;
	}
	auto* hover = m_Root ? m_Root->HitTest(event.m_Position) : nullptr;
	if (hover != m_Hover)
	{
		if (m_Hover) m_Hover->SetHovered(false);
		m_Hover = hover;
		if (m_Hover) m_Hover->SetHovered(true);
		m_Dirty = true;
	}
	Route(event, &Widget::OnPointerMove);
}
void WidgetTree::Wheel(PointerEvent event)
{
	Dispatch dispatch(*this);
	if (m_PopupPointer && !m_PopupClosing) { auto pointer = m_PopupPointer; if (pointer(event)) return; }
	Route(event, &Widget::OnWheel);
}
void WidgetTree::AdvanceFocus(bool backwards)
{
	std::vector<Widget*> candidates;
	std::function<void(Widget&)> visit = [&](Widget& widget)
	{
		if (!widget.IsVisible() || !widget.IsEnabled()) return;
		if (widget.IsFocusable()) candidates.push_back(&widget);
		for (const auto& child : widget.GetChildren()) visit(*child);
	};
	if (m_Root) visit(*m_Root);
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
	if (m_PopupKey && !m_PopupClosing) { auto key = m_PopupKey; if (key(event)) return; }
	for (auto* widget = m_Focus; widget && !event.m_Handled; widget = widget->GetParent()) widget->OnKeyDown(event);
	if (!event.m_Handled && event.m_Key == Key::Tab) { ClosePopup(); AdvanceFocus(event.m_Modifiers.m_Shift); }
}
void WidgetTree::TextInput(std::string_view text) { Dispatch dispatch(*this); if (m_Focus) m_Focus->OnTextInput(text); }
void WidgetTree::TextEditing(std::string_view text, int start, int length)
{
	Dispatch dispatch(*this);
	if (m_Focus) m_Focus->OnTextEditing(text, start, length);
}
void WidgetTree::SetPopup(Widget* owner, std::function<void(Canvas&, float)> paint,
	std::function<bool(PointerEvent&)> pointer, std::function<bool(KeyEvent&)> key)
{
	if (m_PopupOwner != owner) m_PopupProgress.Snap(0);
	m_PopupOwner = owner;
	m_PopupLifetime = owner->ObserveLifetime();
	m_PopupClosing = false;
	m_PopupPaint = std::move(paint);
	m_PopupPointer = std::move(pointer);
	m_PopupKey = std::move(key);
	m_PopupTick = std::chrono::steady_clock::now();
	m_PopupProgress.Retarget(1, owner->MotionDuration(owner->GetTheme().m_Motion.m_PopupDuration));
	m_Animating = m_PopupProgress.IsActive() || m_Animating;
	m_Dirty = true;
}
void WidgetTree::ClosePopup(bool immediate)
{
	if (!m_PopupOwner || (m_PopupClosing && !immediate)) return;
	m_PopupClosing = true;
	const auto duration = !immediate && !m_PopupLifetime.expired() ?
		m_PopupOwner->MotionDuration(m_PopupOwner->GetTheme().m_Motion.m_PopupDuration) : std::chrono::nanoseconds::zero();
	m_PopupTick = std::chrono::steady_clock::now();
	m_PopupProgress.Retarget(0, duration);
	if (!m_PopupProgress.IsActive())
	{
		m_PopupOwner = nullptr;
		m_PopupPaint = {};
		m_PopupPointer = {};
		m_PopupKey = {};
		m_PopupLifetime.reset();
	}
	else m_Animating = true;
	m_Dirty = true;
}
void WidgetTree::LostFocus()
{
	Dispatch dispatch(*this);
	SetFocusVisible(false);
	ClosePopup(true);
	SetFocus(nullptr);
	if (m_Capture) m_Capture->ReleasePointer();
}
void WidgetTree::Tick(std::chrono::steady_clock::time_point now)
{
	Dispatch dispatch(*this);
	m_Animating = m_Root && m_Root->Tick(now);
	if (m_PopupOwner)
	{
		if (m_PopupLifetime.expired()) { ClosePopup(true); return; }
		m_PopupProgress.Retarget(m_PopupClosing ? 0.0f : 1.0f, m_PopupOwner->MotionDuration(m_PopupOwner->GetTheme().m_Motion.m_PopupDuration));
		const float previous = m_PopupProgress.GetValue();
		m_Animating = m_PopupProgress.Advance(std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_PopupTick)) || m_Animating;
		m_PopupTick = now;
		if (previous != m_PopupProgress.GetValue()) m_Dirty = true;
		if (m_PopupClosing && !m_PopupProgress.IsActive()) ClosePopup(true);
	}
}
}
