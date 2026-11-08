#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "WidgetTree.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace Yukiri
{
Widget::Widget() = default;
Widget::~Widget()
{
	m_Lifetime.reset();
	if (m_Tree) m_Tree->Forget(this);
}

void Widget::Attach(Detail::WidgetTree* tree)
{
	if (m_Tree && m_Tree != tree) m_Tree->Forget(this);
	m_Tree = tree;
	for (auto& child : m_Children) child->Attach(tree);
}

Widget& Widget::AddChild(std::unique_ptr<Widget> child)
{
	assert(child && !child->m_Parent && child.get() != this);
	child->m_Parent = this;
	child->Attach(m_Tree);
	auto& observer = *child;
	m_Children.push_back(std::move(child));
	InvalidateLayout();
	return observer;
}

void Widget::RemoveChild(Widget& child)
{
	auto remove = [this, childPointer = &child, lifetime = std::weak_ptr<int>(m_Lifetime)]
	{
		if (lifetime.expired()) return;
		std::erase_if(m_Children, [childPointer](const auto& owned) { return owned.get() == childPointer; });
		InvalidateLayout();
	};
	if (m_Tree) m_Tree->Defer(std::move(remove)); else remove();
}

void Widget::ClearChildren()
{
	std::vector<std::pair<Widget*, std::weak_ptr<int>>> children;
	for (const auto& child : m_Children) children.emplace_back(child.get(), child->m_Lifetime);
	auto clear = [this, children = std::move(children), lifetime = std::weak_ptr<int>(m_Lifetime)]
	{
		if (lifetime.expired()) return;
		std::erase_if(m_Children, [&](const auto& owned)
		{
			return std::ranges::any_of(children, [&](const auto& entry) { return !entry.second.expired() && entry.first == owned.get(); });
		});
		InvalidateLayout();
	};
	if (m_Tree) m_Tree->Defer(std::move(clear)); else clear();
}

const std::vector<std::unique_ptr<Widget>>& Widget::GetChildren() const noexcept { return m_Children; }
Widget* Widget::GetParent() const noexcept { return m_Parent; }
void Widget::SetWidth(float width) { assert(std::isfinite(width) && width >= 0); m_Width = width; InvalidateLayout(); }
void Widget::SetHeight(float height) { assert(std::isfinite(height) && height >= 0); m_Height = height; InvalidateLayout(); }
void Widget::SetMinSize(Size size)
{
	m_Min = {std::max(0.0f, size.m_Width), std::max(0.0f, size.m_Height)};
	m_Max = {std::max(m_Max.m_Width, m_Min.m_Width), std::max(m_Max.m_Height, m_Min.m_Height)};
	InvalidateLayout();
}
void Widget::SetMaxSize(Size size)
{
	m_Max = {std::max(m_Min.m_Width, size.m_Width), std::max(m_Min.m_Height, size.m_Height)};
	InvalidateLayout();
}
void Widget::SetMargin(Insets margin) { m_Margin = margin; InvalidateLayout(); }
void Widget::SetPadding(Insets padding) { m_Padding = padding; InvalidateLayout(); }
void Widget::SetAlignment(Alignment horizontal, Alignment vertical) { m_Horizontal = horizontal; m_Vertical = vertical; InvalidateLayout(); }
void Widget::SetVisible(bool visible)
{
	if (m_Visible == visible) return;
	m_Visible = visible;
	InvalidateLayout();
	if (!visible && m_Tree) m_Tree->Forget(this);
}
void Widget::SetEnabled(bool enabled)
{
	if (m_Enabled == enabled) return;
	m_Enabled = enabled;
	InvalidatePaint();
	if (!enabled && m_Tree) m_Tree->Forget(this);
}
void Widget::SetFocusable(bool focusable) { m_Focusable = focusable; }
void Widget::SetCursor(Cursor cursor) { m_Cursor = cursor; }
void Widget::SetStyle(WidgetStyle style) { m_Style = std::move(style); InvalidateLayout(); }
void Widget::SetThemeOverride(std::optional<Theme> theme) { m_ThemeOverride = std::move(theme); InvalidateLayout(); }
const WidgetStyle& Widget::GetStyle() const noexcept { return m_Style; }
const Theme& Widget::GetTheme() const noexcept
{
	if (m_ThemeOverride) return *m_ThemeOverride;
	if (m_Parent) return m_Parent->GetTheme();
	if (m_Tree) return m_Tree->m_Theme;
	static const Theme Default = Theme::Light();
	return Default;
}
float Widget::GetFontSize() const noexcept { return m_Style.m_FontSize.value_or(GetTheme().m_FontSize); }
float Widget::GetRadius() const noexcept { return m_Style.m_Radius.value_or(GetTheme().m_Radius); }
Color Widget::GetForeground() const noexcept
{
	return IsEnabled() ? m_Style.m_Foreground.value_or(GetTheme().m_Text) : GetTheme().m_MutedText;
}
Insets Widget::GetMargin() const noexcept { return m_Margin; }
Insets Widget::GetPadding() const noexcept { return m_Padding; }
Rect Widget::GetBounds() const noexcept { return m_Bounds; }
Size Widget::GetDesiredSize() const noexcept { return m_Desired; }
Size Widget::GetActualSize() const noexcept { return {m_Bounds.m_Width, m_Bounds.m_Height}; }
bool Widget::IsVisible() const noexcept { return m_Visible; }
bool Widget::IsEnabled() const noexcept { return m_Enabled && (!m_Parent || m_Parent->IsEnabled()); }
bool Widget::IsFocusable() const noexcept { return m_Focusable; }
bool Widget::IsHovered() const noexcept { return m_Hovered; }
bool Widget::HasFocus() const noexcept { return m_Tree && m_Tree->m_Focus == this; }
Cursor Widget::GetCursor() const noexcept { return m_Cursor; }
Window* Widget::GetWindow() const noexcept { return m_Tree ? m_Tree->m_Window : nullptr; }
Point Widget::ToLocal(Point global) const noexcept { return {global.m_X - m_Bounds.m_X, global.m_Y - m_Bounds.m_Y}; }
Rect Widget::GetLocalBounds() const noexcept { return {0, 0, m_Bounds.m_Width, m_Bounds.m_Height}; }
void Widget::Keep(Connection connection) { m_Connections.push_back(std::move(connection)); }

void Widget::Measure(Size available)
{
	if (!m_Visible) { m_Desired = {}; return; }
	available.m_Width = std::max(0.0f, available.m_Width - m_Margin.Horizontal());
	available.m_Height = std::max(0.0f, available.m_Height - m_Margin.Vertical());
	const auto inner = OnMeasure({std::max(0.0f, m_Width.value_or(available.m_Width) - m_Padding.Horizontal()),
		std::max(0.0f, m_Height.value_or(available.m_Height) - m_Padding.Vertical())});
	m_Desired = {std::clamp(m_Width.value_or(inner.m_Width + m_Padding.Horizontal()), m_Min.m_Width, m_Max.m_Width) + m_Margin.Horizontal(),
		std::clamp(m_Height.value_or(inner.m_Height + m_Padding.Vertical()), m_Min.m_Height, m_Max.m_Height) + m_Margin.Vertical()};
}

void Widget::Arrange(Rect slot)
{
	if (!m_Visible) { m_Bounds = {}; return; }
	slot.m_X += m_Margin.m_Left;
	slot.m_Y += m_Margin.m_Top;
	slot.m_Width = std::max(0.0f, slot.m_Width - m_Margin.Horizontal());
	slot.m_Height = std::max(0.0f, slot.m_Height - m_Margin.Vertical());
	const float width = std::clamp(m_Width.value_or(m_Horizontal == Alignment::Stretch ? slot.m_Width :
		std::min(slot.m_Width, m_Desired.m_Width - m_Margin.Horizontal())), m_Min.m_Width, m_Max.m_Width);
	const float height = std::clamp(m_Height.value_or(m_Vertical == Alignment::Stretch ? slot.m_Height :
		std::min(slot.m_Height, m_Desired.m_Height - m_Margin.Vertical())), m_Min.m_Height, m_Max.m_Height);
	auto adjustment = [](Alignment alignment, float extra)
	{
		return alignment == Alignment::Center ? extra * 0.5f : alignment == Alignment::End ? extra : 0.0f;
	};
	m_Bounds = {slot.m_X + adjustment(m_Horizontal, slot.m_Width - width),
		slot.m_Y + adjustment(m_Vertical, slot.m_Height - height), width, height};
	OnArrange({m_Bounds.m_X + m_Padding.m_Left, m_Bounds.m_Y + m_Padding.m_Top,
		std::max(0.0f, width - m_Padding.Horizontal()), std::max(0.0f, height - m_Padding.Vertical())});
}

Size Widget::OnMeasure(Size available)
{
	Size desired;
	for (auto* child : Children())
	{
		child->Measure(available);
		const auto size = child->GetDesiredSize();
		desired.m_Width = std::max(desired.m_Width, size.m_Width);
		desired.m_Height = std::max(desired.m_Height, size.m_Height);
	}
	return desired;
}
void Widget::OnArrange(Rect content) { for (auto* child : Children()) child->Arrange(content); }

void Widget::Render(Canvas& canvas)
{
	if (!m_Visible || m_Bounds.Empty()) return;
	canvas.Save();
	canvas.SetOffset({m_Bounds.m_X, m_Bounds.m_Y});
	canvas.PushClip(GetLocalBounds());
	OnPaint(canvas);
	const auto childClip = GetChildrenClip();
	canvas.PushClip({childClip.m_X - m_Bounds.m_X, childClip.m_Y - m_Bounds.m_Y, childClip.m_Width, childClip.m_Height});
	for (auto* child : Children()) child->Render(canvas);
	canvas.PopClip();
	OnPaintOverlay(canvas);
	canvas.Restore();
}

Widget* Widget::HitTest(Point point)
{
	if (!m_Visible || !IsEnabled() || !m_Bounds.Contains(point)) return nullptr;
	if (GetChildrenClip().Contains(point))
		for (auto child = m_Children.rbegin(); child != m_Children.rend(); ++child)
			if (auto* target = (*child)->HitTest(point)) return target;
	return this;
}
void Widget::InvalidateLayout() { if (m_Tree) m_Tree->m_LayoutDirty = m_Tree->m_Dirty = true; }
void Widget::InvalidatePaint() { if (m_Tree) m_Tree->m_Dirty = true; }
void Widget::RequestFocus() { if (m_Tree) m_Tree->SetFocus(this); }
void Widget::CapturePointer()
{
	if (!m_Tree) return;
	m_Tree->m_Capture = this;
	if (m_Tree->m_PointerCapture) m_Tree->m_PointerCapture(true);
}
void Widget::ReleasePointer()
{
	if (!m_Tree || m_Tree->m_Capture != this) return;
	m_Tree->m_Capture = nullptr;
	if (m_Tree->m_PointerCapture) m_Tree->m_PointerCapture(false);
}
Size Widget::MeasureText(std::string_view text, float fontSize) const
{
	const auto size = fontSize > 0 ? fontSize : GetFontSize();
	return m_Tree && m_Tree->m_MeasureText ? m_Tree->m_MeasureText(text, size) : Size{0, size * 1.5f};
}
void Widget::PaintFocus(Canvas& canvas) const
{
	if (HasFocus()) canvas.StrokeRectangle({2, 2, std::max(0.0f, m_Bounds.m_Width - 4),
		std::max(0.0f, m_Bounds.m_Height - 4)}, GetTheme().m_Accent, 1.5f, std::max(0.0f, GetRadius() - 1));
}
void Widget::OnPaint(Canvas& canvas)
{
	if (m_Style.m_Background) canvas.FillRectangle(GetLocalBounds(), *m_Style.m_Background, GetRadius());
	if (m_Style.m_Border) canvas.StrokeRectangle(GetLocalBounds(), *m_Style.m_Border, 1, GetRadius());
}
void Widget::OnPaintOverlay(Canvas&) {}
void Widget::OnBringIntoView(Rect&) {}
Rect Widget::GetChildrenClip() const noexcept { return m_Bounds; }
void Widget::OnPointerDown(PointerEvent&) {}
void Widget::OnPointerUp(PointerEvent&) {}
void Widget::OnPointerMove(PointerEvent&) {}
void Widget::OnWheel(PointerEvent&) {}
void Widget::OnKeyDown(KeyEvent&) {}
void Widget::OnTextInput(std::string_view) {}
void Widget::OnTextEditing(std::string_view, int, int) {}
void Widget::OnFocusChanged(bool) {}
bool Widget::AcceptsTextInput() const noexcept { return false; }
void Widget::BringIntoView()
{
	Rect rectangle = GetBounds();
	for (auto* parent = m_Parent; parent; parent = parent->m_Parent) parent->OnBringIntoView(rectangle);
}
std::weak_ptr<int> Widget::ObserveLifetime() const noexcept { return m_Lifetime; }
float Widget::GetHoverAmount() const noexcept { return m_HoverAmount; }
void Widget::SetHovered(bool hovered)
{
	if (m_Hovered == hovered) return;
	m_Hovered = hovered;
	m_HoverStart = m_HoverAmount;
	m_HoverChanged = std::chrono::steady_clock::now();
	InvalidatePaint();
}
bool Widget::Tick(std::chrono::steady_clock::time_point now)
{
	const float target = m_Hovered && IsEnabled() ? 1.0f : 0.0f;
	const float elapsed = std::chrono::duration<float>(now - m_HoverChanged).count();
	const float fraction = std::clamp(elapsed / 0.12f, 0.0f, 1.0f);
	const float next = m_HoverStart + (target - m_HoverStart) * fraction;
	if (next != m_HoverAmount) { m_HoverAmount = next; InvalidatePaint(); }
	bool active = fraction < 1 && m_HoverStart != target;
	for (auto& child : m_Children) active = child->Tick(now) || active;
	return active;
}
Color Widget::InteractiveColor(Color resting, Color hovered, Color pressed, bool isPressed) const noexcept
{
	if (!IsEnabled()) return resting;
	if (isPressed) return pressed;
	const float t = m_HoverAmount;
	return {resting.m_R + (hovered.m_R - resting.m_R) * t, resting.m_G + (hovered.m_G - resting.m_G) * t,
		resting.m_B + (hovered.m_B - resting.m_B) * t, resting.m_A + (hovered.m_A - resting.m_A) * t};
}
void Widget::ShowPopup(std::function<void(Canvas&)> paint, std::function<bool(PointerEvent&)> pointer, std::function<bool(KeyEvent&)> key)
{
	if (m_Tree) m_Tree->SetPopup(this, std::move(paint), std::move(pointer), std::move(key));
}
void Widget::ClosePopup() { if (m_Tree && m_Tree->HasPopup(this)) m_Tree->ClosePopup(); }
bool Widget::IsPopupOpen() const noexcept { return m_Tree && m_Tree->HasPopup(this); }
std::vector<Widget*> Widget::Children() const
{
	std::vector<Widget*> snapshot;
	snapshot.reserve(m_Children.size());
	for (const auto& child : m_Children) snapshot.push_back(child.get());
	return snapshot;
}
}
