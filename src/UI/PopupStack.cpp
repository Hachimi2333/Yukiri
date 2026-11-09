#include "WidgetTree.hpp"
#include "Core/Retire.hpp"
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <algorithm>

namespace Yukiri::Detail
{
WidgetTree::Popup* WidgetTree::ParentPopup(const Widget* owner) const
{
	for (auto entry = m_Popups.rbegin(); entry != m_Popups.rend(); ++entry)
		if (!(*entry)->m_Closing && (*entry)->m_Content && Within(owner, (*entry)->m_Content.get())) return entry->get();
	return nullptr;
}
bool WidgetTree::IsPopupDescendant(const Popup& popup, std::uint64_t ancestor) const
{
	if (popup.m_Id == ancestor) return true;
	auto parent = popup.m_ParentId;
	while (parent)
	{
		if (parent == ancestor) return true;
		auto found = std::ranges::find_if(m_Popups, [&](const auto& entry) { return entry->m_Id == parent; });
		if (found == m_Popups.end()) break;
		parent = (*found)->m_ParentId;
	}
	return false;
}
std::chrono::nanoseconds WidgetTree::PopupDuration(const Popup& popup) const
{
	if (popup.m_Lifetime.expired()) return {};
	if (!popup.m_Content && popup.m_Owner) return popup.m_Owner->MotionDuration(popup.m_Owner->GetTheme().m_Motion.m_PopupDuration);
	const auto& motion = popup.m_InheritTheme && popup.m_Owner ? popup.m_Owner->GetTheme().m_Motion :
		popup.m_Content ? popup.m_Content->GetTheme().m_Motion : m_Theme.m_Motion;
	return motion.m_Enabled ? std::max(motion.m_PopupDuration, std::chrono::milliseconds::zero()) : std::chrono::nanoseconds::zero();
}
bool WidgetTree::OwnsPopup(const Widget* owner) const noexcept
{
	if (!owner) return false;
	return std::ranges::any_of(m_Popups, [&](const auto& popup) { return popup->m_Owner == owner || popup->m_Content.get() == owner; });
}
bool WidgetTree::HasPopup(const Widget* owner) const noexcept
{
	if (!owner) return false;
	return std::ranges::any_of(m_Popups, [&](const auto& popup)
	{
		return !popup->m_Closing && (popup->m_Owner == owner || popup->m_Content.get() == owner);
	});
}
Widget* WidgetTree::GetModalRoot() const noexcept
{
	for (auto entry = m_Popups.rbegin(); entry != m_Popups.rend(); ++entry)
		if (!(*entry)->m_Closing && (*entry)->m_Modal) return (*entry)->m_Content.get();
	return nullptr;
}
std::vector<Widget*> WidgetTree::GetPopupRoots() const
{
	std::vector<Widget*> roots;
	for (const auto& popup : m_Popups) if (popup->m_Content && !popup->m_Closing) roots.push_back(popup->m_Content.get());
	return roots;
}
Widget* WidgetTree::FocusRoot() const noexcept
{
	for (auto entry = m_Popups.rbegin(); entry != m_Popups.rend(); ++entry)
		if (!(*entry)->m_Closing && (*entry)->m_Content && (*entry)->m_Interactive) return (*entry)->m_Content.get();
	return m_Root.get();
}
void WidgetTree::FocusFirst(Widget& root)
{
	std::function<Widget*(Widget&)> find = [&](Widget& widget) -> Widget*
	{
		if (!widget.IsVisible() || !widget.IsEnabled()) return nullptr;
		if (widget.IsFocusable() && widget.IsTabStop()) return &widget;
		for (auto* child : widget.Children()) if (auto* found = find(*child)) return found;
		return nullptr;
	};
	if (auto* focus = find(root)) SetFocus(focus, false);
	else SetFocus(root.IsFocusable() ? &root : nullptr, false);
}
void WidgetTree::SetPopup(Widget* owner, std::function<void(Canvas&, float)> paint,
	std::function<bool(PointerEvent&)> pointer, std::function<bool(KeyEvent&)> key)
{
	Dispatch dispatch(*this);
	if (!owner || owner->ObserveLifetime().expired()) return;
	if (owner->m_Tree != this) return;
	for (auto* ancestor = owner; ancestor; ancestor = ancestor->GetParent()) if (!ancestor->IsVisible() || !ancestor->IsEnabled()) return;
	if (auto* modal = GetModalRoot(); modal && !Within(owner, modal)) return;
	HideToolTip();
	Popup* popup = nullptr;
	for (const auto& entry : m_Popups)
		if (entry->m_Owner == owner && !entry->m_Content) { popup = entry.get(); break; }
	if (!popup)
	{
		auto created = std::make_unique<Popup>();
		created->m_Id = m_NextPopupId++;
		if (auto* parent = ParentPopup(owner)) created->m_ParentId = parent->m_Id;
		created->m_Owner = owner;
		created->m_Lifetime = owner->ObserveLifetime();
		popup = created.get();
		const auto parent = created->m_ParentId;
		const auto count = m_Popups.size();
		for (std::size_t index = 0; index < count; ++index)
			if (m_Popups[index]->m_ParentId == parent && !m_Popups[index]->m_Modal) CloseFrom(index, true);
		m_Popups.push_back(std::move(created));
	}
	popup->m_Closing = false;
	popup->m_Paint = std::move(paint);
	popup->m_Pointer = std::move(pointer);
	popup->m_Key = std::move(key);
	popup->m_Tick = std::chrono::steady_clock::now();
	popup->m_Progress.Retarget(1, PopupDuration(*popup));
	m_Animating = popup->m_Progress.IsActive() || m_Animating;
	m_Dirty = true;
}
Result<Widget*> WidgetTree::OpenPopup(Widget* owner, std::unique_ptr<Widget> content, Rect anchor,
	bool modal, bool beside, bool interactive, std::function<void()> canceled)
{
	Dispatch dispatch(*this);
	if (!content || m_Destroying || (m_Window && !m_Window->IsOpen()))
		return std::unexpected(Error{ErrorCode::InvalidArgument, "A popup requires live content and an open window."});
	if (owner && (owner->m_Tree != this || owner->ObserveLifetime().expired()))
		return std::unexpected(Error{ErrorCode::InvalidArgument, "The popup owner belongs to another tree."});
	if (auto* root = GetModalRoot(); owner && root && !Within(owner, root))
		return std::unexpected(Error{ErrorCode::InvalidArgument, "The popup owner is outside the active modal scope."});
	for (auto* ancestor = owner; ancestor; ancestor = ancestor->GetParent())
		if (!ancestor->IsVisible() || !ancestor->IsEnabled())
			return std::unexpected(Error{ErrorCode::InvalidArgument, "The popup owner must be visible and enabled."});
	for (const auto& popup : m_Popups)
		if (popup->m_Closing && popup->m_Content && Within(owner, popup->m_Content.get()))
			return std::unexpected(Error{ErrorCode::InvalidArgument, "Closing popups cannot open children."});
	if (interactive) HideToolTip();
	auto created = std::make_unique<Popup>();
	created->m_Id = m_NextPopupId++;
	if (auto* parent = ParentPopup(owner ? owner : GetModalRoot())) created->m_ParentId = parent->m_Id;
	created->m_Owner = owner;
	created->m_Lifetime = owner ? owner->ObserveLifetime() : content->ObserveLifetime();
	created->m_Anchor = anchor;
	created->m_Modal = modal;
	created->m_Beside = beside;
	created->m_Interactive = interactive;
	created->m_Canceled = std::move(canceled);
	created->m_InheritTheme = owner && !content->m_ThemeOverride;
	created->m_InheritFont = owner && !content->m_Style.m_Font;
	content->Attach(this);
	created->m_Content = std::move(content);
	created->m_Tick = std::chrono::steady_clock::now();
	created->m_Progress.Retarget(1, PopupDuration(*created));
	const auto count = m_Popups.size();
	const auto parent = created->m_ParentId;
	for (std::size_t index = 0; index < count; ++index)
		if (interactive && !m_Popups[index]->m_Modal && (modal || m_Popups[index]->m_ParentId == parent)) CloseFrom(index, true);
	created->m_PreviousFocus = m_Focus;
	if (m_Focus) created->m_PreviousFocusLifetime = m_Focus->ObserveLifetime();
	auto* observer = created->m_Content.get();
	m_Popups.push_back(std::move(created));
	if (interactive)
	{
		CancelPointer();
		FocusFirst(*observer);
	}
	m_LayoutDirty = m_Dirty = true;
	m_Animating = true;
	if (!HasPopup(observer)) return std::unexpected(Error{ErrorCode::InvalidArgument, "The popup closed while receiving focus."});
	return observer;
}
void WidgetTree::CloseFrom(std::size_t first, bool immediate, bool notify)
{
	if (first >= m_Popups.size()) return;
	const auto ancestor = m_Popups[first]->m_Id;
	bool restore = false;
	for (const auto& popup : m_Popups)
		if (IsPopupDescendant(*popup, ancestor) && popup->m_Content && Within(m_Focus, popup->m_Content.get())) restore = true;
	auto* previous = m_Popups[first]->m_PreviousFocus;
	const auto previousLifetime = m_Popups[first]->m_PreviousFocusLifetime;
	std::vector<std::function<void()>> callbacks;
	for (const auto& popup : m_Popups)
	{
		if (!IsPopupDescendant(*popup, ancestor) || (popup->m_Closing && !immediate)) continue;
		popup->m_Closing = true;
		popup->m_Tick = std::chrono::steady_clock::now();
		popup->m_Progress.Retarget(0, immediate ? std::chrono::nanoseconds::zero() : PopupDuration(*popup));
		if (popup->m_Canceled)
		{
			if (notify) callbacks.push_back(std::move(popup->m_Canceled));
			else popup->m_Canceled = {};
		}
		m_Animating = popup->m_Progress.IsActive() || m_Animating;
	}
	if (restore)
	{
		CancelPointer();
		SetFocus(!previousLifetime.expired() ? previous : nullptr, false);
		if (!m_Focus) if (auto* root = FocusRoot()) FocusFirst(*root);
	}
	for (auto& callback : callbacks) callback();
	m_Dirty = true;
	PrunePopups();
}
void WidgetTree::ClosePopup(bool immediate)
{
	Dispatch dispatch(*this);
	const auto count = m_Popups.size();
	for (std::size_t index = 0; index < count; ++index) if (!m_Popups[index]->m_Modal) CloseFrom(index, immediate);
}
void WidgetTree::ClosePopupFor(const Widget* owner, bool immediate)
{
	Dispatch dispatch(*this);
	for (std::size_t index = 0; index < m_Popups.size(); ++index)
		if (m_Popups[index]->m_Content.get() == owner) { CloseFrom(index, immediate); return; }
	for (std::size_t index = 0; index < m_Popups.size(); ++index)
		if (m_Popups[index]->m_Owner == owner) { CloseFrom(index, immediate); return; }
}
void WidgetTree::CloseAllPopups(bool notify)
{
	Dispatch dispatch(*this);
	const auto count = m_Popups.size();
	for (std::size_t index = 0; index < count; ++index) CloseFrom(index, true, notify);
}
void WidgetTree::PrunePopups()
{
	Defer([this]
	{
		RetireMatching(m_Popups, [](const Popup& popup)
		{
			return (popup.m_Closing && !popup.m_Progress.IsActive()) || popup.m_Lifetime.expired();
		});
	});
}
Rect WidgetTree::VisibleBounds(const Popup& popup) const
{
	if (popup.m_Modal) return popup.m_Bounds;
	const auto height = popup.m_Bounds.m_Height * popup.m_Progress.GetValue();
	return {popup.m_Bounds.m_X, popup.m_Up ? popup.m_Bounds.Bottom() - height : popup.m_Bounds.m_Y, popup.m_Bounds.m_Width, height};
}
void WidgetTree::LayoutPopups(Size size, Point origin)
{
	const Rect viewport{origin.m_X, origin.m_Y, size.m_Width, size.m_Height};
	std::vector<Popup*> snapshot;
	for (const auto& popup : m_Popups) snapshot.push_back(popup.get());
	for (auto* popup : snapshot)
	{
		if (!popup->m_Content || popup->m_Lifetime.expired()) continue;
		auto& content = *popup->m_Content;
		if (popup->m_InheritTheme) content.m_ThemeOverride = popup->m_Owner->GetTheme();
		if (popup->m_InheritFont) content.m_Style.m_Font = popup->m_Owner->GetFont();
		const Size maximum{std::max(0.0f, size.m_Width - 16), std::max(0.0f, size.m_Height - 16)};
		content.Measure(maximum);
		const auto desired = content.GetDesiredSize();
		const float width = std::min(desired.m_Width, maximum.m_Width);
		const float height = std::min(desired.m_Height, maximum.m_Height);
		Rect anchor = popup->m_Anchor;
		if (popup->m_Owner)
		{
			const auto bounds = popup->m_Owner->GetBounds();
			anchor.m_X += bounds.m_X;
			anchor.m_Y += bounds.m_Y;
		}
		float x = popup->m_Modal ? viewport.m_X + (size.m_Width - width) * 0.5f : popup->m_Beside ? anchor.Right() + 2 : anchor.m_X;
		float y = popup->m_Modal ? viewport.m_Y + (size.m_Height - height) * 0.5f : popup->m_Beside ? anchor.m_Y : anchor.Bottom() + 2;
		popup->m_Up = !popup->m_Modal && !popup->m_Beside && y + height > viewport.Bottom() - 8;
		if (popup->m_Up) y = anchor.m_Y - height - 2;
		if (popup->m_Beside && x + width > viewport.Right() - 8) x = anchor.m_X - width - 2;
		x = std::clamp(x, viewport.m_X + std::min(8.0f, size.m_Width * 0.5f), std::max(viewport.m_X + 8, viewport.Right() - width - 8));
		y = std::clamp(y, viewport.m_Y + std::min(8.0f, size.m_Height * 0.5f), std::max(viewport.m_Y + 8, viewport.Bottom() - height - 8));
		popup->m_Bounds = {x, y, width, height};
		content.Arrange(popup->m_Bounds);
	}
}
void WidgetTree::PaintPopups(Canvas& canvas)
{
	std::vector<Popup*> snapshot;
	for (const auto& popup : m_Popups) snapshot.push_back(popup.get());
	for (auto* popup : snapshot)
	{
		if (popup->m_Lifetime.expired()) continue;
		canvas.Save();
		canvas.SetOffset({0, 0});
		if (popup->m_Content)
		{
			if (popup->m_Modal) canvas.FillRectangle({m_LastOrigin.m_X, m_LastOrigin.m_Y, m_LastSize.m_Width, m_LastSize.m_Height},
				Color::FromHex(0x000000).WithAlpha(0.25f * popup->m_Progress.GetValue()));
			canvas.PushClip(VisibleBounds(*popup));
			canvas.SetOpacity(canvas.GetOpacity() * popup->m_Progress.GetValue());
			popup->m_Content->Render(canvas);
			canvas.PopClip();
		}
		else if (popup->m_Paint)
		{
			auto paint = popup->m_Paint;
			paint(canvas, popup->m_Progress.GetValue());
		}
		canvas.Restore();
	}
}
bool WidgetTree::PopupPointer(PointerEvent& event, bool down)
{
	if (m_Capture) return false;
	for (std::size_t index = m_Popups.size(); index > 0; --index)
	{
		auto& popup = *m_Popups[index - 1];
		if (popup.m_Closing || popup.m_Lifetime.expired()) continue;
		if (!popup.m_Interactive) { if (down) CloseFrom(index - 1, true); continue; }
		if (popup.m_Pointer)
		{
			auto pointer = popup.m_Pointer;
			if (pointer(event)) return true;
			if (down) CloseFrom(index - 1, false);
			continue;
		}
		if (popup.m_Content && VisibleBounds(popup).Contains(event.m_Position)) return popup.m_Content->HitTest(event.m_Position) == nullptr;
		if (popup.m_Modal) return true;
		if (down) CloseFrom(index - 1, false);
	}
	return false;
}
bool WidgetTree::PopupKey(KeyEvent& event)
{
	for (auto entry = m_Popups.rbegin(); entry != m_Popups.rend(); ++entry)
	{
		if ((*entry)->m_Closing || !(*entry)->m_Interactive) continue;
		if ((*entry)->m_Key) { auto key = (*entry)->m_Key; return key(event); }
		if ((*entry)->m_Content) return false;
	}
	return false;
}
void WidgetTree::TickPopups(std::chrono::steady_clock::time_point now)
{
	std::vector<Popup*> snapshot;
	for (const auto& popup : m_Popups) snapshot.push_back(popup.get());
	for (auto* popup : snapshot)
	{
		if (popup->m_Lifetime.expired()) continue;
		popup->m_Progress.Retarget(popup->m_Closing ? 0.0f : 1.0f, PopupDuration(*popup));
		const auto previous = popup->m_Progress.GetValue();
		m_Animating = popup->m_Progress.Advance(std::chrono::duration_cast<std::chrono::nanoseconds>(now - popup->m_Tick)) || m_Animating;
		popup->m_Tick = now;
		if (previous != popup->m_Progress.GetValue()) m_Dirty = true;
		if (popup->m_Content) m_Animating = popup->m_Content->Tick(now) || m_Animating;
	}
	PrunePopups();
}
}
