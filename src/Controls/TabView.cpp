#include <Yukiri/Controls/TabView.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include "Text/Unicode.hpp"
#include "Visuals.hpp"

namespace Yukiri
{
TabView::TabView() { SetFocusable(true); }
Widget& TabView::AddTab(std::string title, std::unique_ptr<Widget> content)
{
	content->SetVisible(GetChildren().size() == m_Selected);
	m_TabWeights.emplace_back(GetChildren().size() == m_Selected ? 1.0f : 0.0f);
	m_Titles.push_back(Detail::SanitizeUtf8(title, true));
	return AddChild(std::move(content));
}
Result<void> TabView::SetSelectedIndex(std::size_t index)
{
	if (index >= GetChildren().size()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Tab index is outside the content list."});
	if (m_Selected == index) return {};
	m_Selected = index;
	RequestAnimationFrame();
	const auto lifetime = ObserveLifetime();
	const auto children = Children();
	for (std::size_t i = 0; i < children.size(); ++i)
	{
		children[i]->SetVisible(i == index);
		if (lifetime.expired()) return {};
	}
	InvalidateLayout();
	RequestFocus();
	m_SelectionChanged.Emit(index);
	return {};
}
std::size_t TabView::GetSelectedIndex() const noexcept { return m_Selected; }
std::vector<Rect> TabView::Headers() const
{
	std::vector<Rect> headers;
	float x = 4;
	for (std::size_t i = 0; i < GetChildren().size(); ++i)
	{
		const auto text = i < m_Titles.size() ? std::string_view(m_Titles[i]) : std::string_view("Tab");
		const float width = std::max(64.0f, MeasureText(text).m_Width + 24);
		headers.push_back({x, 0, width, 38});
		x += width + 4;
	}
	return headers;
}
Size TabView::OnMeasure(Size available)
{
	Size desired;
	m_Headers = Headers();
	for (const auto& header : m_Headers) desired.m_Width = std::max(desired.m_Width, header.Right());
	for (auto* child : Children())
	{
		child->Measure({available.m_Width, std::max(0.0f, available.m_Height - 44)});
		desired.m_Width = std::max(desired.m_Width, child->GetDesiredSize().m_Width);
		desired.m_Height = std::max(desired.m_Height, child->GetDesiredSize().m_Height);
	}
	desired.m_Height += 44;
	return desired;
}
void TabView::OnArrange(Rect content)
{
	for (auto* child : Children()) child->Arrange({content.m_X, content.m_Y + 44, content.m_Width, std::max(0.0f, content.m_Height - 44)});
}
Rect TabView::GetFocusBounds() const noexcept
{
	if (m_Selected >= m_Headers.size()) return {};
	const auto header = m_Headers[m_Selected];
	return Rect{header.m_X + 2, 3, std::max(0.0f, header.m_Width - 4), 30}.Intersect(GetLocalBounds());
}
bool TabView::OnAnimate(std::chrono::nanoseconds delta)
{
	const auto headers = Headers();
	if (m_Selected >= headers.size()) return false;
	const auto rectangle = headers[m_Selected];
	const auto duration = MotionDuration(GetTheme().m_Motion.m_SelectionDuration);
	m_IndicatorX.Retarget(rectangle.m_X + 10, duration);
	m_IndicatorWidth.Retarget(rectangle.m_Width - 20, duration);
	bool active = m_IndicatorX.Advance(delta);
	active = m_IndicatorWidth.Advance(delta) || active;
	for (std::size_t i = 0; i < m_TabWeights.size(); ++i)
	{
		m_TabWeights[i].Retarget(i == m_Selected ? 1.0f : 0.0f, MotionDuration(GetTheme().m_Motion.m_StateDuration));
		active = m_TabWeights[i].Advance(delta) || active;
	}
	return active;
}
void TabView::OnPaint(Canvas& canvas)
{
	Widget::OnPaint(canvas);
	const auto& theme = GetTheme();
	canvas.FillRectangle({0, 37, GetActualSize().m_Width, 1}, theme.m_Border);
	const auto headers = Headers();
	for (std::size_t i = 0; i < headers.size(); ++i)
	{
		const auto& rectangle = headers[i];
		const float weight = i < m_TabWeights.size() ? m_TabWeights[i].GetValue() : 0;
		canvas.FillRectangle({rectangle.m_X, 2, rectangle.m_Width, 34}, theme.m_Hover.WithAlpha(weight), 5);
		canvas.DrawText(i < m_Titles.size() ? m_Titles[i] : "Tab", {rectangle.m_X + 12, 8}, Detail::Mix(GetForeground(), theme.m_Accent, weight), GetFontSize());
	}
	canvas.FillRectangle({m_IndicatorX.GetValue(), 35, m_IndicatorWidth.GetValue(), 2}, theme.m_Accent, 1);
	PaintFocus(canvas);
}
void TabView::OnPointerDown(PointerEvent& event)
{
	if (event.m_Button != PointerButton::Left) return;
	const auto headers = Headers();
	for (std::size_t i = 0; i < headers.size(); ++i)
		if (headers[i].Contains(ToLocal(event.m_Position))) { event.m_Handled = true; auto selected = SetSelectedIndex(i); (void)selected; return; }
}
void TabView::OnKeyDown(KeyEvent& event)
{
	if (!HasFocus() || GetChildren().empty()) return;
	std::size_t index = m_Selected;
	if (event.m_Key == Key::Right) index = (index + 1) % GetChildren().size();
	else if (event.m_Key == Key::Left) index = (index + GetChildren().size() - 1) % GetChildren().size();
	else if (event.m_Key == Key::Home) index = 0;
	else if (event.m_Key == Key::End) index = GetChildren().size() - 1;
	else return;
	event.m_Handled = true;
	auto selected = SetSelectedIndex(index);
	(void)selected;
}
}
