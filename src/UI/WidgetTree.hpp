#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <functional>
#include <Yukiri/Core/Dispatcher.hpp>

namespace Yukiri::Detail
{
class WidgetTree
{
public:
	~WidgetTree();
	void SetRoot(std::unique_ptr<Widget> root);
	void Layout(Size size, Point origin = {});
	void Paint(Canvas& canvas);
	void PointerDown(PointerEvent event);
	void PointerUp(PointerEvent event);
	void PointerMove(PointerEvent event);
	void Wheel(PointerEvent event);
	void KeyDown(KeyEvent event);
	void KeyUp(KeyEvent event);
	void CancelPointer();
	void SetCapture(Widget* widget);
	void TextInput(std::string_view text);
	void TextEditing(std::string_view text, int start, int length);
	void SetFocus(Widget* widget, bool bringIntoView = true);
	void Forget(Widget* widget);
	void Defer(std::function<void()> action);
	void SetPopup(Widget* owner, std::function<void(Canvas&, float)> paint,
		std::function<bool(PointerEvent&)> pointer, std::function<bool(KeyEvent&)> key);
	void ClosePopup(bool immediate = false);
	void ClosePopupFor(const Widget* owner, bool immediate = false);
	void CloseAllPopups(bool notify = true);
	[[nodiscard]] Result<Widget*> OpenPopup(Widget* owner, std::unique_ptr<Widget> content, Rect anchor,
		bool modal = false, bool beside = false, bool interactive = true, std::function<void()> canceled = {});
	[[nodiscard]] Widget* GetModalRoot() const noexcept;
	[[nodiscard]] std::vector<Widget*> GetPopupRoots() const;
	void Invoke(std::function<void()> action);
	void LostFocus();
	void GainedFocus();
	void Tick(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
	[[nodiscard]] bool OwnsPopup(const Widget* owner) const noexcept;
	[[nodiscard]] bool HasPopup(const Widget* owner) const noexcept;
	std::unique_ptr<Widget> m_Root;
	Widget* m_Focus = nullptr;
	Widget* m_Capture = nullptr;
	Widget* m_Hover = nullptr;
	Window* m_Window = nullptr;
	Theme m_Theme = Theme::Light();
	bool m_Dirty = true;
	bool m_LayoutDirty = true;
	bool m_Animating = false;
	std::function<void()> m_EndTextInput;
	std::function<void(bool)> m_PointerCapture;
	std::function<Size(std::string_view, float, const Font&)> m_MeasureText;
	std::vector<std::shared_ptr<Command>> m_Commands;
	std::function<bool(KeyEvent&)> m_ApplicationCommand;
	Signal<KeyEvent&> m_CommandBindings;

private:
	friend class AccessibilityTree;
	std::shared_ptr<int> m_Lifetime = std::make_shared<int>(0);
	std::uint64_t m_AccessibilityStructureVersion = 0;
	class Dispatch
	{
	public:
		explicit Dispatch(WidgetTree& tree);
		~Dispatch();
	private:
		WidgetTree& m_Tree;
	};
	void Flush();
	[[nodiscard]] Widget* Target(Point position);
	[[nodiscard]] Widget* HitTarget(Point position);
	void Route(PointerEvent& event, void (Widget::*handler)(PointerEvent&));
	void AdvanceFocus(bool backwards);
	void SetFocusVisible(bool visible);
	static bool Within(const Widget* candidate, const Widget* ancestor);
	Widget* FocusRoot() const noexcept;
	void FocusFirst(Widget& root);
	struct Popup
	{
		std::uint64_t m_Id = 0;
		std::uint64_t m_ParentId = 0;
		Widget* m_Owner = nullptr;
		std::weak_ptr<int> m_Lifetime;
		std::unique_ptr<Widget> m_Content;
		std::function<void(Canvas&, float)> m_Paint;
		std::function<bool(PointerEvent&)> m_Pointer;
		std::function<bool(KeyEvent&)> m_Key;
		std::function<void()> m_Canceled;
		Widget* m_PreviousFocus = nullptr;
		std::weak_ptr<int> m_PreviousFocusLifetime;
		Rect m_Anchor;
		Rect m_Bounds;
		bool m_Modal = false;
		bool m_Beside = false;
		bool m_Interactive = true;
		bool m_Closing = false;
		bool m_Up = false;
		bool m_InheritTheme = false;
		bool m_InheritFont = false;
		Transition m_Progress;
		std::chrono::steady_clock::time_point m_Tick{};
	};
	void CloseFrom(std::size_t first, bool immediate, bool notify = true);
	void PrunePopups();
	void LayoutPopups(Size size, Point origin);
	void PaintPopups(Canvas& canvas);
	void TickPopups(std::chrono::steady_clock::time_point now);
	bool PopupPointer(PointerEvent& event, bool down);
	bool PopupKey(KeyEvent& event);
	Rect VisibleBounds(const Popup& popup) const;
	Popup* ParentPopup(const Widget* owner) const;
	bool IsPopupDescendant(const Popup& popup, std::uint64_t ancestor) const;
	std::chrono::nanoseconds PopupDuration(const Popup& popup) const;
	void UpdateToolTip();
	void HideToolTip();
	bool m_FocusVisible = false;
	Widget* m_LastFocus = nullptr;
	friend class Yukiri::Widget;
	int m_DispatchDepth = 0;
	bool m_Flushing = false;
	std::vector<std::function<void()>> m_Deferred;
	std::vector<std::unique_ptr<Popup>> m_Popups;
	std::uint64_t m_NextPopupId = 1;
	bool m_Destroying = false;
	Timer m_ToolTipTimer;
	Widget* m_ToolTipOwner = nullptr;
	Widget* m_ToolTipPopup = nullptr;
	std::weak_ptr<int> m_ToolTipLifetime;
	Point m_PointerPosition;
	Size m_LastSize{-1, -1};
	Point m_LastOrigin;
};
}
