#include <Yukiri/Controls/NumberBox.hpp>
#include <Yukiri/Controls/Button.hpp>
#include <charconv>
#include <cmath>

namespace Yukiri
{
class NumberBox::Editor : public TextBox
{
public:
	explicit Editor(NumberBox& owner) : m_Owner(owner), m_Lifetime(ObserveLifetime(owner)) {}
	void OnFocusChanged(bool focused) override { TextBox::OnFocusChanged(focused); if (!focused && !m_Lifetime.expired()) { const auto committed = m_Owner.Commit(); (void)committed; } }
private:
	NumberBox& m_Owner;
	std::weak_ptr<int> m_Lifetime;
};
NumberBox::NumberBox(double value)
{
	SetFocusable(true);
	SetTabStop(false);
	m_Editor = &AddChild<Editor>(*this);
	m_Up = &AddChild<Button>("+");
	m_Down = &AddChild<Button>("\xE2\x88\x92");
	WidgetStyle stepStyle;
	stepStyle.m_FontSize = 10.0f;
	stepStyle.m_Radius = 2.0f;
	m_Up->SetStyle(stepStyle); m_Down->SetStyle(stepStyle);
	m_Up->SetTabStop(false); m_Down->SetTabStop(false);
	m_Up->SetToolTip("Increase value"); m_Down->SetToolTip("Decrease value");
	m_Up->Keep(m_Up->m_Clicked.Connect([this] { StepBy(1); }));
	m_Down->Keep(m_Down->m_Clicked.Connect([this] { StepBy(-1); }));
	m_Editor->Keep(m_Editor->m_Submitted.Connect([this] { const auto committed = Commit(); (void)committed; }));
	m_Editor->Keep(m_Editor->m_TextChanged.Connect([this](const auto&) { UpdateValidity(); }));
	m_Editor->Keep(m_Editor->m_EditStateChanged.Connect([this]
	{
		m_Up->SetEnabled(!IsReadOnly() && m_Value < m_Maximum);
		m_Down->SetEnabled(!IsReadOnly() && m_Value > m_Minimum);
	}));
	m_Value = std::isfinite(value) ? value : 0;
	FormatValue();
}
Result<void> NumberBox::SetRange(double minimum, double maximum)
{
	if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum >= maximum) return std::unexpected(Error{ErrorCode::InvalidArgument, "The numeric range must be finite and increasing."});
	m_Minimum = minimum; m_Maximum = maximum;
	SetValue(m_Value);
	return {};
}
Result<void> NumberBox::SetStep(double step)
{
	if (!std::isfinite(step) || step <= 0) return std::unexpected(Error{ErrorCode::InvalidArgument, "The numeric step must be positive and finite."});
	m_Step = step; return {};
}
Result<void> NumberBox::SetFormat(Format format, int precision)
{
	if (precision < 0 || precision > 15 || (format == Format::General && precision == 0)) return std::unexpected(Error{ErrorCode::InvalidArgument, "Numeric precision must be 0..15, or 1..15 for general format."});
	m_Format = format; m_Precision = precision;
	FormatValue(); return {};
}
void NumberBox::SetValue(double value)
{
	if (!std::isfinite(value)) return;
	Dispatch([&]
	{
		value = std::clamp(value, m_Minimum, m_Maximum);
		const bool changed = m_Value != value;
		m_Value = value;
		FormatValue();
		if (changed) m_ValueChanged.Emit(value);
	});
}
double NumberBox::GetValue() const noexcept { return m_Value; }
void NumberBox::SetReadOnly(bool readOnly) { m_Editor->SetReadOnly(readOnly); }
bool NumberBox::IsReadOnly() const noexcept { return m_Editor->IsReadOnly(); }
double NumberBox::GetMinimum() const noexcept { return m_Minimum; }
double NumberBox::GetMaximum() const noexcept { return m_Maximum; }
double NumberBox::GetStep() const noexcept { return m_Step; }
bool NumberBox::IsValid() const noexcept { return m_Valid; }
TextBox& NumberBox::GetEditor() noexcept { return *m_Editor; }
std::optional<double> NumberBox::Parse() const
{
	std::string_view text = m_Editor->GetText();
	while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
	while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
	if (!text.empty() && text.front() == '+') text.remove_prefix(1);
	if (text.empty()) return {};
	double value = 0;
	const auto result = std::from_chars(text.data(), text.data() + text.size(), value, std::chars_format::general);
	if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || !std::isfinite(value) || value < m_Minimum || value > m_Maximum) return {};
	return value;
}
void NumberBox::UpdateValidity()
{
	const bool valid = Parse().has_value();
	const bool changed = valid != m_Valid;
	m_Valid = valid;
	auto style = m_Editor->GetStyle();
	style.m_Border = valid ? std::nullopt : std::optional(Color::FromHex(0xD94D4D));
	m_Editor->SetStyle(style);
	if (changed) m_ValidationChanged.Emit(valid);
}
void NumberBox::FormatValue()
{
	char buffer[512];
	const auto format = m_Format == Format::Fixed ? std::chars_format::fixed : m_Format == Format::Scientific ? std::chars_format::scientific : std::chars_format::general;
	auto result = std::to_chars(std::begin(buffer), std::end(buffer), m_Value, format, m_Precision);
	double displayed = 0;
	if (result.ec == std::errc{})
	{
		const auto parsed = std::from_chars(buffer, result.ptr, displayed, std::chars_format::general);
		if (parsed.ec != std::errc{} || displayed < m_Minimum || displayed > m_Maximum)
			result = std::to_chars(std::begin(buffer), std::end(buffer), m_Value, std::chars_format::general, 17);
	}
	if (result.ec == std::errc{}) m_Editor->SetText(std::string_view(buffer, static_cast<std::size_t>(result.ptr - buffer)));
	UpdateValidity();
	m_Up->SetEnabled(!IsReadOnly() && m_Value < m_Maximum);
	m_Down->SetEnabled(!IsReadOnly() && m_Value > m_Minimum);
}
bool NumberBox::Commit()
{
	if (IsReadOnly()) return false;
	bool committed = false;
	Dispatch([&] { const auto value = Parse(); UpdateValidity(); if (value) { SetValue(*value); committed = true; } });
	return committed;
}
void NumberBox::Revert() { Dispatch([&] { FormatValue(); }); }
void NumberBox::StepBy(int steps)
{
	if (IsReadOnly()) return;
	Dispatch([&]
	{
		const auto base = Parse().value_or(m_Value);
		const auto value = std::clamp(static_cast<long double>(base) + static_cast<long double>(m_Step) * steps, static_cast<long double>(m_Minimum), static_cast<long double>(m_Maximum));
		SetValue(static_cast<double>(value));
	});
}
void NumberBox::OnPreviewKeyDown(KeyEvent& event)
{
	if (event.m_Modifiers.m_Alt || event.m_Modifiers.m_Control || event.m_Modifiers.m_Super) return;
	if (event.m_Key == Key::Up || event.m_Key == Key::Down || event.m_Key == Key::PageUp || event.m_Key == Key::PageDown)
	{
		event.m_Handled = true;
		StepBy(event.m_Key == Key::Up ? 1 : event.m_Key == Key::Down ? -1 : event.m_Key == Key::PageUp ? 10 : -10);
	}
	else if (event.m_Key == Key::Escape) { event.m_Handled = true; Revert(); }
}
void NumberBox::OnFocusChanged(bool focused) { if (focused) m_Editor->RequestFocus(); }
Size NumberBox::OnMeasure(Size available)
{
	m_Editor->Measure(available); m_Up->Measure(available); m_Down->Measure(available);
	return {220, m_Editor->GetDesiredSize().m_Height};
}
void NumberBox::OnArrange(Rect content)
{
	const float buttons = std::min(28.0f, content.m_Width / 3);
	m_Editor->Arrange({content.m_X, content.m_Y, std::max(0.0f, content.m_Width - buttons - 4), content.m_Height});
	m_Up->Arrange({content.Right() - buttons, content.m_Y, buttons, content.m_Height / 2});
	m_Down->Arrange({content.Right() - buttons, content.m_Y + content.m_Height / 2, buttons, content.m_Height / 2});
}
}
