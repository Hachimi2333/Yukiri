#include "TextBoxState.hpp"
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <cmath>

namespace Yukiri
{
Size TextBox::OnMeasure(Size)
{
	return m_Impl->m_Multiline ? Size{320, 180} : Size{220, std::max(GetTheme().m_ControlHeight, GetFontSize() * 1.5f + 12)};
}
void TextBox::OnArrange(Rect) { PrepareLayout(); }
void TextBox::PrepareLayout() const
{
	auto& state = *m_Impl;
	if (state.m_DirtyVisual || state.m_VisualRevision != state.m_Editor.m_Revision)
	{
		auto visual = Detail::BuildTextVisual(state.m_Composition.BuildDisplay(state.m_Editor), state.m_Password);
		state.m_DocumentDirty = state.m_DocumentDirty || visual.m_Display.m_Text != state.m_Visual.m_Display.m_Text;
		state.m_Visual = std::move(visual);
		state.m_DirtyVisual = false;
		state.m_VisualRevision = state.m_Editor.m_Revision;
	}
	else if (!IsComposing())
	{
		auto& display = state.m_Visual.m_Display;
		display.m_Caret = state.m_Visual.ToVisual(state.m_Editor.m_Caret);
		display.m_SelectionStart = state.m_Visual.ToVisual(state.m_Editor.SelectionStart());
		display.m_SelectionEnd = state.m_Visual.ToVisual(state.m_Editor.SelectionEnd());
	}
	const auto scale = GetWindow() ? GetWindow()->GetScale() : 1.0f;
	const auto sourceGeneration = GetWindow() ? GetWindow()->GetFontGeneration() : 0;
	if (state.m_LayoutFont != GetFont() || state.m_LayoutFontSize != GetFontSize() || state.m_LayoutScale != scale || state.m_LayoutSourceGeneration != sourceGeneration)
	{
		state.m_LayoutFont = GetFont(); state.m_LayoutFontSize = GetFontSize(); state.m_LayoutScale = scale;
		state.m_LayoutSourceGeneration = sourceGeneration;
		++state.m_LayoutGeneration;
		state.m_DocumentDirty = true;
	}
	const auto size = GetActualSize();
	if (!state.m_DocumentDirty && state.m_PreparedSize.m_Width == size.m_Width && state.m_PreparedSize.m_Height == size.m_Height && state.m_PreparedWrap == state.m_Wrap) return;
	state.m_PreparedSize = size;
	state.m_PreparedWrap = state.m_Wrap;
	state.m_DocumentDirty = false;
	const float lineHeight = std::max(1.0f, MeasureText("Mg").m_Height * (state.m_Multiline ? 1.08f : 1.0f));
	const float width = std::max(0.0f, size.m_Width - 20);
	const float height = state.m_Multiline ? std::max(0.0f, size.m_Height - 12) : lineHeight;
	const bool hadVerticalBar = state.m_Multiline && state.m_Document.GetSize().m_Height > height;
	state.m_Viewport = {10, state.m_Multiline ? 6.0f : (size.m_Height - lineHeight) / 2, std::max(0.0f, width - (hadVerticalBar ? 12 : 0)), height};
	const auto measure = [&](std::string_view value) { return MeasureText(value); };
	Detail::TextCarets carets;
	if (GetWindow()) carets = [&](std::string_view value) { return GetWindow()->GetTextCarets(value, GetFontSize(), GetFont()); };
	state.m_Document.Update(state.m_Visual.m_Display.m_Text, state.m_Viewport.m_Width, state.m_Multiline && state.m_Wrap, lineHeight, state.m_LayoutGeneration, measure, carets);
	const bool needsVerticalBar = state.m_Multiline && state.m_Document.GetSize().m_Height > height;
	if (hadVerticalBar != needsVerticalBar)
	{
		state.m_Viewport.m_Width = std::max(0.0f, width - (needsVerticalBar ? 12 : 0));
		state.m_Document.Update(state.m_Visual.m_Display.m_Text, state.m_Viewport.m_Width, state.m_Wrap, lineHeight, state.m_LayoutGeneration, measure, carets);
	}
	if (state.m_Multiline && !state.m_Wrap && state.m_Document.GetSize().m_Width > state.m_Viewport.m_Width)
	{
		state.m_Viewport.m_Height = std::max(0.0f, height - 12);
		if (state.m_Document.GetSize().m_Height > state.m_Viewport.m_Height) state.m_Viewport.m_Width = std::max(0.0f, width - 12);
	}
	const auto extent = state.m_Document.GetSize();
	const Point maximum{std::max(0.0f, extent.m_Width - state.m_Viewport.m_Width + 2), state.m_Multiline ? std::max(0.0f, extent.m_Height - state.m_Viewport.m_Height) : 0};
	const auto previousOffset = state.m_Scroll;
	state.m_Scroll = {std::clamp(state.m_Scroll.m_X, 0.0f, maximum.m_X), std::clamp(state.m_Scroll.m_Y, 0.0f, maximum.m_Y)};
	if (previousOffset.m_X != state.m_Scroll.m_X) state.m_ScrollX.Snap(state.m_Scroll.m_X);
	if (previousOffset.m_Y != state.m_Scroll.m_Y) state.m_ScrollY.Snap(state.m_Scroll.m_Y);
	state.m_Target = {std::clamp(state.m_Target.m_X, 0.0f, maximum.m_X), std::clamp(state.m_Target.m_Y, 0.0f, maximum.m_Y)};
}
Point TextBox::GetOffset() const noexcept { return m_Impl->m_Scroll; }
void TextBox::SetOffset(Point offset)
{
	if (!std::isfinite(offset.m_X) || !std::isfinite(offset.m_Y)) return;
	PrepareLayout();
	const auto extent = m_Impl->m_Document.GetSize();
	const auto viewport = m_Impl->m_Viewport;
	m_Impl->m_Scroll = {std::clamp(offset.m_X, 0.0f, std::max(0.0f, extent.m_Width - viewport.m_Width + 2)),
		m_Impl->m_Multiline ? std::clamp(offset.m_Y, 0.0f, std::max(0.0f, extent.m_Height - viewport.m_Height)) : 0};
	m_Impl->m_Target = m_Impl->m_Scroll;
	m_Impl->m_ScrollX.Snap(m_Impl->m_Scroll.m_X); m_Impl->m_ScrollY.Snap(m_Impl->m_Scroll.m_Y);
	m_Impl->m_EnsureCaret = false;
	InvalidatePaint();
}
void TextBox::ScrollTo(Point offset)
{
	PrepareLayout();
	const auto extent = m_Impl->m_Document.GetSize();
	const auto viewport = m_Impl->m_Viewport;
	m_Impl->m_Target = {std::clamp(offset.m_X, 0.0f, std::max(0.0f, extent.m_Width - viewport.m_Width + 2)),
		m_Impl->m_Multiline ? std::clamp(offset.m_Y, 0.0f, std::max(0.0f, extent.m_Height - viewport.m_Height)) : 0};
	const auto duration = MotionDuration(GetTheme().m_Motion.m_ScrollDuration);
	m_Impl->m_ScrollX.Retarget(m_Impl->m_Target.m_X, duration); m_Impl->m_ScrollY.Retarget(m_Impl->m_Target.m_Y, duration);
	m_Impl->m_EnsureCaret = false;
	if (m_Impl->m_ScrollX.IsActive() || m_Impl->m_ScrollY.IsActive()) RequestAnimationFrame();
	else SetOffset(m_Impl->m_Target);
}
void TextBox::EnsureCaretVisible()
{
	if (!m_Impl->m_EnsureCaret) return;
	const auto& display = m_Impl->m_Visual.m_Display;
	const auto line = m_Impl->m_Document.LineAt(display.m_Caret, m_Impl->m_Upstream && !display.m_Composing);
	const float x = m_Impl->m_Document.CaretX(line, display.m_Caret);
	const float y = static_cast<float>(line) * m_Impl->m_Document.GetLineHeight();
	const auto viewport = m_Impl->m_Viewport;
	auto offset = m_Impl->m_Scroll;
	if (x < offset.m_X) offset.m_X = x;
	else if (x > offset.m_X + viewport.m_Width - 2) offset.m_X = x - viewport.m_Width + 2;
	if (y < offset.m_Y) offset.m_Y = y;
	else if (y + m_Impl->m_Document.GetLineHeight() > offset.m_Y + viewport.m_Height) offset.m_Y = y + m_Impl->m_Document.GetLineHeight() - viewport.m_Height;
	SetOffset(offset);
}
Rect TextBox::GetCaretBounds() const
{
	const auto bounds = GetBounds();
	if (GetActualSize().m_Width <= 0) return {bounds.m_X + 10, bounds.m_Y + 6, 1, GetFontSize() * 1.5f};
	PrepareLayout();
	const auto& display = m_Impl->m_Visual.m_Display;
	const auto line = m_Impl->m_Document.LineAt(display.m_Caret, m_Impl->m_Upstream && !display.m_Composing);
	return {bounds.m_X + m_Impl->m_Viewport.m_X + m_Impl->m_Document.CaretX(line, display.m_Caret) - m_Impl->m_Scroll.m_X,
		bounds.m_Y + m_Impl->m_Viewport.m_Y + static_cast<float>(line) * m_Impl->m_Document.GetLineHeight() - m_Impl->m_Scroll.m_Y, 1.25f, m_Impl->m_Document.GetLineHeight()};
}
std::vector<Rect> TextBox::GetTextBounds(std::size_t start, std::size_t end) const
{
	PrepareLayout();
	std::vector<Rect> result;
	if (IsPassword()) return result;
	const auto first = m_Impl->m_Visual.ToVisual(std::min(start, end)), last = m_Impl->m_Visual.ToVisual(std::max(start, end));
	const auto bounds = GetBounds(), viewport = m_Impl->m_Viewport;
	const auto& lines = m_Impl->m_Document.GetLines();
	const auto visible = m_Impl->m_Document.Visible(m_Impl->m_Scroll.m_Y, viewport.m_Height);
	for (std::size_t index = visible.first; index < visible.second; ++index)
	{
		const auto& line = lines[index];
		if (last < line.m_Start || first > line.m_End) continue;
		const float left = m_Impl->m_Document.CaretX(index, std::max(first, line.m_Start));
		const float right = m_Impl->m_Document.CaretX(index, std::min(last, line.m_End));
		const Rect rect{bounds.m_X + viewport.m_X + left - m_Impl->m_Scroll.m_X, bounds.m_Y + viewport.m_Y + static_cast<float>(index) * m_Impl->m_Document.GetLineHeight() - m_Impl->m_Scroll.m_Y, std::max(1.0f, right - left), m_Impl->m_Document.GetLineHeight()};
		const auto clipped = rect.Intersect({bounds.m_X + viewport.m_X, bounds.m_Y + viewport.m_Y, viewport.m_Width, viewport.m_Height});
		if (!clipped.Empty()) result.push_back(clipped);
	}
	return result;
}
std::size_t TextBox::OffsetAt(Point point) const
{
	PrepareLayout();
	const auto local = ToLocal(point);
	const float y = local.m_Y - m_Impl->m_Viewport.m_Y + m_Impl->m_Scroll.m_Y;
	const auto index = static_cast<std::size_t>(std::max(0.0f, std::floor(y / m_Impl->m_Document.GetLineHeight())));
	const auto offset = m_Impl->m_Document.Hit(index, local.m_X - m_Impl->m_Viewport.m_X + m_Impl->m_Scroll.m_X);
	return m_Impl->m_Visual.ToSource(offset);
}
std::size_t TextBox::HitTestOffset(Point global) const { return OffsetAt(global); }
Rect TextBox::VerticalThumb() const
{
	const auto size = m_Impl->m_Document.GetSize();
	const auto viewport = m_Impl->m_Viewport;
	if (!m_Impl->m_Multiline || size.m_Height <= viewport.m_Height || viewport.m_Height <= 0) return {};
	const float height = std::min(viewport.m_Height, std::max(24.0f, viewport.m_Height * viewport.m_Height / size.m_Height));
	return {GetActualSize().m_Width - 9, viewport.m_Y + (viewport.m_Height - height) * m_Impl->m_Scroll.m_Y / std::max(1.0f, size.m_Height - viewport.m_Height), 5, height};
}
Rect TextBox::HorizontalThumb() const
{
	const auto size = m_Impl->m_Document.GetSize();
	const auto viewport = m_Impl->m_Viewport;
	if (!m_Impl->m_Multiline || m_Impl->m_Wrap || size.m_Width <= viewport.m_Width || viewport.m_Width <= 0) return {};
	const float width = std::min(viewport.m_Width, std::max(24.0f, viewport.m_Width * viewport.m_Width / size.m_Width));
	return {viewport.m_X + (viewport.m_Width - width) * m_Impl->m_Scroll.m_X / std::max(1.0f, size.m_Width - viewport.m_Width), GetActualSize().m_Height - 9, width, 5};
}
void TextBox::OnWheel(PointerEvent& event)
{
	if (!m_Impl->m_Multiline) return;
	const auto previous = m_Impl->m_Target;
	ScrollTo({previous.m_X - (event.m_Modifiers.m_Shift ? event.m_WheelY : event.m_WheelX) * 40, previous.m_Y - (event.m_Modifiers.m_Shift ? 0 : event.m_WheelY) * 40});
	event.m_Handled = previous.m_X != m_Impl->m_Target.m_X || previous.m_Y != m_Impl->m_Target.m_Y;
}
bool TextBox::OnAnimate(std::chrono::nanoseconds delta)
{
	if (!m_Impl->m_Dragging && !m_Impl->m_ScrollX.IsActive() && !m_Impl->m_ScrollY.IsActive()) return false;
	PrepareLayout();
	const auto duration = MotionDuration(GetTheme().m_Motion.m_ScrollDuration);
	m_Impl->m_ScrollX.Retarget(m_Impl->m_Target.m_X, duration); m_Impl->m_ScrollY.Retarget(m_Impl->m_Target.m_Y, duration);
	bool active = m_Impl->m_ScrollX.Advance(delta);
	active = m_Impl->m_ScrollY.Advance(delta) || active;
	const Point offset{m_Impl->m_ScrollX.GetValue(), m_Impl->m_ScrollY.GetValue()};
	if (offset.m_X != m_Impl->m_Scroll.m_X || offset.m_Y != m_Impl->m_Scroll.m_Y) { m_Impl->m_Scroll = offset; InvalidatePaint(); }
	if (m_Impl->m_Dragging)
	{
		const auto local = ToLocal(m_Impl->m_DragPointer);
		const auto viewport = m_Impl->m_Viewport;
		const float dx = local.m_X < viewport.m_X ? local.m_X - viewport.m_X : local.m_X > viewport.Right() ? local.m_X - viewport.Right() : 0;
		const float dy = local.m_Y < viewport.m_Y ? local.m_Y - viewport.m_Y : local.m_Y > viewport.Bottom() ? local.m_Y - viewport.Bottom() : 0;
		if (dx || dy)
		{
			const float seconds = std::clamp(std::chrono::duration<float>(delta).count(), 0.0f, 0.1f);
			const auto previous = m_Impl->m_Scroll;
			SetOffset({previous.m_X + std::clamp(dx * 8, -900.0f, 900.0f) * seconds, previous.m_Y + std::clamp(dy * 8, -900.0f, 900.0f) * seconds});
			PointerEvent move{m_Impl->m_DragPointer, PointerButton::None, {}};
			OnPointerMove(move);
			active = previous.m_X != m_Impl->m_Scroll.m_X || previous.m_Y != m_Impl->m_Scroll.m_Y || active;
		}
	}
	return active;
}
void TextBox::OnPaint(Canvas& canvas)
{
	PrepareLayout();
	EnsureCaretVisible();
	const auto& theme = GetTheme();
	canvas.FillRectangle(GetLocalBounds(), InteractiveColor(GetStyle().m_Background.value_or(theme.m_Surface), theme.m_Hover, theme.m_Pressed, false), GetRadius());
	canvas.StrokeRectangle(GetLocalBounds(), GetStyle().m_Border.value_or(IsFocusVisible() ? theme.m_Accent : theme.m_Border), IsFocusVisible() ? 1.5f : 1, GetRadius());
	const auto& display = m_Impl->m_Visual.m_Display;
	const auto viewport = m_Impl->m_Viewport;
	const auto& document = m_Impl->m_Document;
	const auto visible = document.Visible(m_Impl->m_Scroll.m_Y, viewport.m_Height);
	canvas.PushClip(viewport);
	const float left = viewport.m_X - m_Impl->m_Scroll.m_X;
	for (std::size_t index = visible.first; index < visible.second; ++index)
	{
		const auto& line = document.GetLines()[index];
		const float top = viewport.m_Y + static_cast<float>(index) * document.GetLineHeight() - m_Impl->m_Scroll.m_Y;
		if (display.m_SelectionStart < display.m_SelectionEnd && display.m_SelectionStart <= line.m_End && display.m_SelectionEnd > line.m_Start)
		{
			const float start = document.CaretX(index, std::max(display.m_SelectionStart, line.m_Start));
			const float end = display.m_SelectionEnd > line.m_End ? std::max(line.m_Width + 6, viewport.m_Width + m_Impl->m_Scroll.m_X) : document.CaretX(index, display.m_SelectionEnd);
			canvas.FillRectangle({left + start, top, end - start, document.GetLineHeight()}, theme.m_Selection, 1);
		}
		canvas.DrawText(std::string_view(display.m_Text).substr(line.m_Start, line.m_End - line.m_Start), {left, top}, GetForeground(), GetFontSize());
		if (display.m_Composing && display.m_CompositionStart <= line.m_End && display.m_CompositionEnd > line.m_Start)
		{
			const float start = document.CaretX(index, std::max(display.m_CompositionStart, line.m_Start));
			const float end = document.CaretX(index, std::min(display.m_CompositionEnd, line.m_End));
			canvas.FillRectangle({left + start, top + document.GetLineHeight() - 2, end - start, 1}, theme.m_Accent);
		}
	}
	if (display.m_Text.empty() && !HasFocus()) canvas.DrawText(m_Impl->m_Placeholder, {viewport.m_X, viewport.m_Y}, theme.m_MutedText, GetFontSize());
	const auto caret = GetCaretBounds();
	const auto bounds = GetBounds();
	if (HasFocus() && (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_Impl->m_LastInteraction).count() / 500) % 2 == 0)
		canvas.FillRectangle({caret.m_X - bounds.m_X, caret.m_Y - bounds.m_Y + 1, caret.m_Width, std::max(0.0f, caret.m_Height - 2)}, theme.m_Text);
	canvas.PopClip();
	canvas.FillRectangle(VerticalThumb(), theme.m_MutedText.WithAlpha(0.55f), 3);
	canvas.FillRectangle(HorizontalThumb(), theme.m_MutedText.WithAlpha(0.55f), 3);
	if (HasFocus() && GetWindow() && !IsReadOnly()) GetWindow()->StartTextInput(caret);
}
}
