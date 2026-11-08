#pragma once

#include <Yukiri/Core/Geometry.hpp>
#include <Yukiri/Core/Signal.hpp>
#include <Yukiri/Styling/Theme.hpp>
#include <Yukiri/UI/Input.hpp>

#include <limits>
#include <memory>
#include <optional>
#include <string_view>
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
	void SetCursor(Cursor cursor);
	void SetStyle(WidgetStyle style);
	void SetThemeOverride(std::optional<Theme> theme);
	[[nodiscard]] const WidgetStyle& GetStyle() const noexcept;
	[[nodiscard]] const Theme& GetTheme() const noexcept;
	[[nodiscard]] float GetFontSize() const noexcept;
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
	[[nodiscard]] bool IsHovered() const noexcept;
	[[nodiscard]] bool HasFocus() const noexcept;
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
	[[nodiscard]] Size MeasureText(std::string_view text, float fontSize = 0) const;
	[[nodiscard]] Point ToLocal(Point global) const noexcept;

	virtual void OnPointerDown(PointerEvent& event);
	virtual void OnPointerUp(PointerEvent& event);
	virtual void OnPointerMove(PointerEvent& event);
	virtual void OnWheel(PointerEvent& event);
	virtual void OnKeyDown(KeyEvent& event);
	virtual void OnTextInput(std::string_view text);
	virtual void OnTextEditing(std::string_view text, int start, int length);
	virtual void OnFocusChanged(bool focused);
	[[nodiscard]] virtual bool AcceptsTextInput() const noexcept;

protected:
	virtual Size OnMeasure(Size available);
	virtual void OnArrange(Rect content);
	virtual void OnPaint(Canvas& canvas);
	virtual void OnPaintOverlay(Canvas& canvas);
	[[nodiscard]] Rect GetLocalBounds() const noexcept;
	void PaintFocus(Canvas& canvas) const;

private:
	friend class Detail::WidgetTree;
	friend class Grid;
	void Attach(Detail::WidgetTree* tree);
	std::vector<std::unique_ptr<Widget>> m_Children;
	std::vector<Connection> m_Connections;
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
	bool m_Hovered = false;
	Cursor m_Cursor = Cursor::Arrow;
	WidgetStyle m_Style;
	std::optional<Theme> m_ThemeOverride;
};
}
