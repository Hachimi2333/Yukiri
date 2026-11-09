#include <Yukiri/Controls/Dialog.hpp>
#include <Yukiri/Controls/Button.hpp>
#include <Yukiri/Controls/Label.hpp>
#include <Yukiri/Layout/Grid.hpp>
#include <Yukiri/Layout/WrapPanel.hpp>
#include <Yukiri/Platform/Window.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <algorithm>
#include <cassert>

namespace Yukiri
{
Dialog::Dialog(std::string title)
{
	SetPadding(Insets(16));
	SetFocusable(true);
	SetTabStop(false);
	m_Layout = &AddChild<Grid>();
	m_Layout->SetRows({GridLength::Auto(), GridLength::Star(), GridLength::Auto()});
	m_Layout->SetColumns({GridLength::Star()});
	m_Layout->SetSpacing(0, 12);
	m_Title = &m_Layout->AddChild<Label>(std::move(title));
	m_Title->SetWrap(true);
	WidgetStyle titleStyle;
	titleStyle.m_FontSize = 18.0f;
	titleStyle.m_Font = Font{"", 600, false};
	m_Title->SetStyle(std::move(titleStyle));
	SetContent(std::make_unique<Widget>());
	m_ButtonPanel = &m_Layout->AddChild<WrapPanel>();
	m_ButtonPanel->SetAlignment(Alignment::End, Alignment::Start);
	auto assigned = m_Layout->SetCell(*m_ButtonPanel, 2, 0);
	assert(assigned);
	SetButtons({{"OK", Result::Ok}, {"Cancel", Result::Cancel}});
}
Yukiri::Result<Dialog*> Dialog::Show(Window& window, std::unique_ptr<Dialog> dialog)
{
	if (!dialog || dialog->m_Complete) return std::unexpected(Error{ErrorCode::InvalidArgument, "An incomplete dialog is required."});
	auto* observer = dialog.get();
	const auto lifetime = dialog->ObserveLifetime();
	auto opened = window.OpenPopup(nullptr, std::move(dialog), {}, true, false, true, [observer, lifetime]
	{
		if (!lifetime.expired()) observer->Dispatch([observer] { observer->CompleteImpl(Result::Cancel, true); });
	});
	if (!opened) return std::unexpected(opened.error());
	return observer;
}
void Dialog::SetTitle(std::string_view title) { m_Title->SetText(title); }
Widget* Dialog::GetContent() const noexcept { return m_ContentLifetime.expired() ? nullptr : m_Content; }
Widget& Dialog::SetContent(std::unique_ptr<Widget> content)
{
	assert(content);
	if (auto* old = GetContent()) m_Layout->RemoveChild(*old);
	m_Content = &m_Layout->InsertChild(1, std::move(content));
	m_ContentLifetime = ObserveLifetime(*m_Content);
	auto assigned = m_Layout->SetCell(*m_Content, 1, 0);
	assert(assigned);
	InvalidateLayout();
	return *m_Content;
}
void Dialog::SetButtons(std::vector<std::pair<std::string, Result>> buttons, Result defaultResult, std::optional<Result> cancelResult)
{
	m_DefaultResult = defaultResult;
	m_CancelResult = cancelResult;
	m_ButtonPanel->ClearChildren();
	m_Actions.clear();
	for (auto& [text, result] : buttons)
	{
		auto& button = m_ButtonPanel->AddChild<Button>(std::move(text));
		button.SetAccent(result == defaultResult);
		button.Keep(button.m_Clicked.Connect([this, result] { Complete(result); }));
		m_Actions.push_back({&button, ObserveLifetime(button), result});
	}
	InvalidateLayout();
}
Button* Dialog::GetButton(Result result) const noexcept
{
	for (const auto& action : m_Actions) if (action.m_Result == result && !action.m_Lifetime.expired()) return action.m_Button;
	return nullptr;
}
void Dialog::Complete(Result result) { Dispatch([this, result] { CompleteImpl(result, false); }); }
void Dialog::CompleteImpl(Result result, bool force)
{
	if (m_Complete || m_Completing) return;
	m_Completing = true;
	CloseRequestEvent request;
	if (!force) m_CloseRequested.Emit(result, request);
	m_Completing = false;
	if (request.m_Cancel) return;
	m_Complete = true;
	ClosePopup();
	m_Completed.Emit(result);
}
Size Dialog::OnMeasure(Size available)
{
	const float width = std::min(448.0f, available.m_Width);
	m_Layout->Measure({width, available.m_Height});
	const auto desired = m_Layout->GetDesiredSize();
	return {std::min(width, std::max(200.0f, desired.m_Width)), desired.m_Height};
}
void Dialog::OnArrange(Rect content)
{
	m_Layout->Measure({content.m_Width, content.m_Height});
	m_Layout->Arrange(content);
}
void Dialog::OnPaint(Canvas& canvas)
{
	canvas.FillRectangle(GetLocalBounds(), GetStyle().m_Background.value_or(GetTheme().m_Surface), GetRadius());
	canvas.StrokeRectangle(GetLocalBounds(), GetStyle().m_Border.value_or(GetTheme().m_Border), 1, GetRadius());
}
void Dialog::OnPreviewKeyDown(KeyEvent& event)
{
	if (m_Complete) { event.m_Handled = true; return; }
	if (event.m_Key == Key::Escape)
	{
		event.m_Handled = true;
		if (m_CancelResult) Complete(*m_CancelResult);
	}
	else if (event.m_Key == Key::Enter && !event.m_Modifiers.m_Control && !event.m_Modifiers.m_Alt)
	{
		// Editors and focused buttons retain their own Return behavior.
		std::function<bool(const Widget&)> accepts = [&](const Widget& widget)
		{
			if (widget.HasFocus() && widget.AcceptsReturn()) return true;
			for (const auto& child : widget.GetChildren()) if (accepts(*child)) return true;
			return false;
		};
		if (accepts(*this)) return;
		if (auto* button = GetButton(m_DefaultResult); button && button->IsEnabled())
		{
			event.m_Handled = true;
			KeyEvent activate{Key::Enter, {}};
			button->OnKeyDown(activate);
		}
	}
}
}
