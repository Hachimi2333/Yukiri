#pragma once

#include <Yukiri/Core/Geometry.hpp>
#include <Yukiri/Core/Command.hpp>
#include <Yukiri/Core/Detail/Transition.hpp>
#include <Yukiri/Core/Signal.hpp>
#include <Yukiri/Styling/Theme.hpp>
#include <Yukiri/UI/Input.hpp>

#include <limits>
#include <chrono>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

namespace Yukiri
{
class Canvas;
class Window;
namespace Detail { class WidgetTree; }
enum class Alignment { Stretch, Start, Center, End };

class Widget
{
public:
	Widget();
	virtual ~Widget();
	Widget(const Widget&) = delete;
	Widget& operator=(const Widget&) = delete;
	Widget& AddChild(std::unique_ptr<Widget> child);
	Widget& InsertChild(std::size_t index, std::unique_ptr<Widget> child);
	template<typename T, typename... Args>
	T& AddChild(Args&&... args)
	{
		auto child = std::make_unique<T>(std::forward<Args>(args)...);
		auto& observer = *child;
		AddChild(std::move(child));
		return observer;
	}
	void RemoveChild(Widget& child);
	void ClearChildren();
	[[nodiscard]] const std::vector<std::unique_ptr<Widget>>& GetChildren() const noexcept;
	[[nodiscard]] Widget* GetParent() const noexcept;
	void SetWidth(float width);
	void SetHeight(float height);
	void SetMinSize(Size size);
	void SetMaxSize(Size size);
	void SetMargin(Insets margin);
	void SetPadding(Insets padding);
	void SetAlignment(Alignment horizontal, Alignment vertical);
	void SetVisible(bool visible);
	void SetEnabled(bool enabled);
	void SetFocusable(bool focusable);
	void SetTabStop(bool tabStop);
	void SetHitTestVisible(bool visible);
	void SetCursor(Cursor cursor);
	void SetToolTip(std::string text);
	[[nodiscard]] std::string_view GetToolTip() const noexcept;
	void SetStyle(WidgetStyle style);
	void SetThemeOverride(std::optional<Theme> theme);
	[[nodiscard]] const WidgetStyle& GetStyle() const noexcept;
	[[nodiscard]] const Theme& GetTheme() const noexcept;
	[[nodiscard]] float GetFontSize() const noexcept;
	[[nodiscard]] const Font& GetFont() const noexcept;
	[[nodiscard]] float GetRadius() const noexcept;
	[[nodiscard]] Color GetForeground() const noexcept;
	[[nodiscard]] Insets GetMargin() const noexcept;
	[[nodiscard]] Insets GetPadding() const noexcept;
	[[nodiscard]] Rect GetBounds() const noexcept;
	[[nodiscard]] Size GetDesiredSize() const noexcept;
	[[nodiscard]] Size GetActualSize() const noexcept;
	[[nodiscard]] bool IsVisible() const noexcept;
	[[nodiscard]] bool IsEnabled() const noexcept;
	[[nodiscard]] bool IsFocusable() const noexcept;
	[[nodiscard]] virtual bool IsTabStop() const noexcept;
	[[nodiscard]] bool IsHovered() const noexcept;
	[[nodiscard]] bool HasFocus() const noexcept;
	[[nodiscard]] bool IsFocusVisible() const noexcept;
	[[nodiscard]] Cursor GetCursor() const noexcept;
	[[nodiscard]] Window* GetWindow() const noexcept;
	void Measure(Size available);
	void Arrange(Rect slot);
	void Render(Canvas& canvas);
	[[nodiscard]] Widget* HitTest(Point point);
	void InvalidateLayout();
	void InvalidatePaint();
	void RequestFocus();
	void CapturePointer();
	void ReleasePointer();
	void Keep(Connection connection);
	void AddCommand(std::shared_ptr<Command> command);
	void RemoveCommand(const Command& command);
	void BringIntoView();
	[[nodiscard]] Size MeasureText(std::string_view text, float fontSize = 0) const;
	[[nodiscard]] Point ToLocal(Point global) const noexcept;

	virtual void OnPointerDown(PointerEvent& event);
	virtual void OnPointerUp(PointerEvent& event);
	virtual void OnPointerMove(PointerEvent& event);
	virtual void OnPointerEnter();
	virtual void OnPointerLeave();
	virtual void OnPointerCaptureLost();
	virtual void OnWheel(PointerEvent& event);
	virtual void OnContextMenu(PointerEvent& event);
	virtual void OnKeyDown(KeyEvent& event);
	virtual void OnPreviewKeyDown(KeyEvent& event);
	virtual void OnKeyUp(KeyEvent& event);
	virtual void OnTextInput(std::string_view text);
	virtual void OnTextEditing(std::string_view text, int start, int length);
	virtual void OnFocusChanged(bool focused);
	[[nodiscard]] virtual bool AcceptsTextInput() const noexcept;
	[[nodiscard]] virtual bool AcceptsReturn() const noexcept;
	[[nodiscard]] virtual bool IsComposing() const noexcept;
	Signal<Point> m_ContextMenuRequested;

protected:
	virtual bool OnAnimate(std::chrono::nanoseconds delta);
	void RequestAnimationFrame();
	[[nodiscard]] bool CanAnimate() const noexcept;
	[[nodiscard]] std::chrono::nanoseconds MotionDuration(std::chrono::milliseconds duration) const noexcept;
	virtual Size OnMeasure(Size available);
	virtual void OnArrange(Rect content);
	virtual void OnPaint(Canvas& canvas);
	virtual void OnPaintOverlay(Canvas& canvas);
	virtual void OnBringIntoView(Rect& rectangle);
	[[nodiscard]] virtual Rect GetChildrenClip() const noexcept;
	[[nodiscard]] virtual Rect GetFocusBounds() const noexcept;
	[[nodiscard]] Rect GetLocalBounds() const noexcept;
	void PaintFocus(Canvas& canvas) const;
	[[nodiscard]] Color InteractiveColor(Color resting, Color hovered, Color pressed, bool isPressed) noexcept;
	[[nodiscard]] std::weak_ptr<int> ObserveLifetime() const noexcept;
	[[nodiscard]] static std::weak_ptr<int> ObserveLifetime(const Widget& widget) noexcept;
	void Dispatch(std::function<void()> action);
	[[nodiscard]] float GetHoverAmount() const noexcept;
	[[nodiscard]] std::vector<Widget*> Children() const;
	void ShowPopup(std::function<void(Canvas&)> paint, std::function<bool(PointerEvent&)> pointer,
		std::function<bool(KeyEvent&)> key);
	void ShowPopup(std::function<void(Canvas&, float)> paint, std::function<bool(PointerEvent&)> pointer,
		std::function<bool(KeyEvent&)> key);
	void ClosePopup(bool immediate = false);
	[[nodiscard]] Result<Widget*> ShowPopupContent(std::unique_ptr<Widget> content, Rect anchor, bool beside = false, bool interactive = true);
	[[nodiscard]] bool IsPopupOpen() const noexcept;

private:
	friend class Detail::WidgetTree;
	friend class Grid;
	friend class ScrollView;
	void TranslateSubtree(Point delta);
	void BringFocusIntoView();
	void Attach(Detail::WidgetTree* tree);
	void SetHovered(bool hovered);
	bool Tick(std::chrono::steady_clock::time_point now);
	std::vector<std::unique_ptr<Widget>> m_Children;
	std::vector<Connection> m_Connections;
	std::vector<std::shared_ptr<Command>> m_Commands;
	std::shared_ptr<int> m_Lifetime = std::make_shared<int>(0);
	Widget* m_Parent = nullptr;
	Detail::WidgetTree* m_Tree = nullptr;
	Rect m_Bounds;
	Size m_Desired;
	std::optional<float> m_Width;
	std::optional<float> m_Height;
	Size m_Min;
	Size m_Max{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
	Insets m_Margin;
	Insets m_Padding;
	Alignment m_Horizontal = Alignment::Stretch;
	Alignment m_Vertical = Alignment::Stretch;
	bool m_Visible = true;
	bool m_Enabled = true;
	bool m_Focusable = false;
	bool m_TabStop = true;
	bool m_HitTestVisible = true;
	bool m_Hovered = false;
	Detail::Transition m_HoverTransition;
	Detail::Transition m_PressTransition;
	bool m_AnimationRequested = false;
	bool m_HasArranged = false;
	std::chrono::steady_clock::time_point m_LastAnimationTick{};
	Cursor m_Cursor = Cursor::Arrow;
	std::string m_ToolTip;
	WidgetStyle m_Style;
	std::optional<Theme> m_ThemeOverride;
};
}
