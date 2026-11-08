#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <functional>

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
	void TextInput(std::string_view text);
	void TextEditing(std::string_view text, int start, int length);
	void SetFocus(Widget* widget);
	void Forget(Widget* widget);
	void Defer(std::function<void()> action);
	void SetPopup(Widget* owner, std::function<void(Canvas&)> paint,
		std::function<bool(PointerEvent&)> pointer, std::function<bool(KeyEvent&)> key);
	void ClosePopup();
	void LostFocus();
	void Tick(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
	[[nodiscard]] bool HasPopup(const Widget* owner) const noexcept { return m_PopupOwner == owner; }
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
	std::function<Size(std::string_view, float)> m_MeasureText;

private:
	class Dispatch;
	void Flush();
	[[nodiscard]] Widget* Target(Point position);
	void Route(PointerEvent& event, void (Widget::*handler)(PointerEvent&));
	void AdvanceFocus(bool backwards);
	int m_DispatchDepth = 0;
	bool m_Flushing = false;
	std::vector<std::function<void()>> m_Deferred;
	Widget* m_PopupOwner = nullptr;
	std::function<void(Canvas&)> m_PopupPaint;
	std::function<bool(PointerEvent&)> m_PopupPointer;
	std::function<bool(KeyEvent&)> m_PopupKey;
	Size m_LastSize{-1, -1};
	Point m_LastOrigin;
};
}
