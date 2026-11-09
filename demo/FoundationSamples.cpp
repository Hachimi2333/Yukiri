#include "FoundationSamples.hpp"
#include "Card.hpp"
#include "ResponsivePanel.hpp"
#include <Yukiri/Yukiri.hpp>
#include <array>
#include <string_view>
#include <format>

namespace YukiriDemo
{
using namespace Yukiri;
std::unique_ptr<Widget> MakeFoundationSamples()
{
	auto page = std::make_unique<ResponsivePanel>(340.0f, 2, 16.0f);
	auto& fields = page->AddChild<Card>();
	fields.AddChild<Label>("Additional inputs / 更多输入");
	auto& password = fields.AddChild<TextBox>(); password.SetPassword(true); password.SetPlaceholder("Password / 密码");
	AccessibilityInfo passwordInfo; passwordInfo.m_Name = "Password / 密码"; password.SetAccessibilityInfo(std::move(passwordInfo));
	fields.AddChild<Label>("Password copy/cut is blocked / 密码禁止复制与剪切");
	auto& number = fields.AddChild<NumberBox>(12.5); (void)number.SetRange(0, 100); (void)number.SetStep(0.5); (void)number.SetFormat(NumberBox::Format::Fixed, 1);
	AccessibilityInfo numberInfo; numberInfo.m_Name = "Quantity / 数量"; number.SetAccessibilityInfo(std::move(numberInfo));
	auto& output = fields.AddChild<Label>("Quantity / 数量: 12.5");
	fields.Keep(number.m_ValueChanged.Connect([&output](double value) { output.SetText(std::format("Quantity / 数量: {:.1f}", value)); }));
	auto& progress = fields.AddChild<ProgressBar>(); progress.SetIndeterminate(true);
	fields.AddChild<Label>("Indeterminate progress / 不确定进度");
	auto& text = fields.AddChild<TextArea>("Multiple lines / 多行编辑\nTry Chinese IME, selection, undo and word wrap.\n试试中文输入法、跨行选择、撤销和自动换行。"); text.SetHeight(150);
	AccessibilityInfo textInfo; textInfo.m_Name = "Multiline sample / 多行示例"; text.SetAccessibilityInfo(std::move(textInfo));
	auto& resources = page->AddChild<Card>();
	resources.AddChild<Label>("Images, fonts and borders / 图片、字体与边框");
	constexpr std::string_view svg = R"(<svg xmlns="http://www.w3.org/2000/svg" width="240" height="100" viewBox="0 0 240 100"><rect width="240" height="100" rx="12" fill="#3674ea"/><circle cx="55" cy="50" r="30" fill="#ffffff" fill-opacity=".85"/><circle cx="115" cy="50" r="30" fill="#35b78a" fill-opacity=".85"/><circle cx="175" cy="50" r="30" fill="#e84c80" fill-opacity=".85"/></svg>)";
	auto vector = Image::FromMemory({reinterpret_cast<const std::uint8_t*>(svg.data()), svg.size()});
	if (vector) { auto& image = resources.AddChild<ImageView>(*vector); image.SetHeight(100); image.SetStretch(ImageView::Stretch::Uniform); image.SetToolTip("Self-contained SVG rasterized at the window DPI."); }
	const std::array<std::uint8_t, 16> pixels{54, 116, 234, 255, 53, 183, 138, 255, 232, 76, 128, 128, 255, 255, 255, 0};
	auto raster = Image::FromRgba(2, 2, pixels);
	if (raster) { auto& image = resources.AddChild<ImageView>(*raster); image.SetHeight(64); image.SetStretch(ImageView::Stretch::Uniform); }
	auto& border = resources.AddChild<Border>(); border.SetPadding(Insets(12));
	auto styled = std::make_unique<StackPanel>(Orientation::Vertical, 6.0f);
	auto& font = styled->AddChild<Label>("Segoe UI Semibold / 字体选择");
	WidgetStyle fontStyle; fontStyle.m_Font = Font{"Segoe UI", 600, false}; fontStyle.m_FontSize = 18.0f; font.SetStyle(fontStyle);
	styled->AddChild<Label>("Border uses the current theme / 边框跟随主题"); border.SetContent(std::move(styled));
	return page;
}
}
