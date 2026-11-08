#include "Gallery.hpp"
#include "Card.hpp"
#include "CanvasSample.hpp"
#include "ResponsivePanel.hpp"
#include <format>

namespace YukiriDemo
{
using namespace Yukiri;
namespace
{
Label& Heading(Widget& parent, std::string text, float size)
{
	auto& label = parent.AddChild<Label>(std::move(text));
	WidgetStyle style;
	style.m_FontSize = size;
	label.SetStyle(style);
	return label;
}
}

Result<Window*> OpenGallery(Application& application, WindowDecoration decoration, ThemeMode theme, bool visible)
{
	WindowConfig config;
	config.m_Title = decoration == WindowDecoration::Native ? "Yukiri - Widget Gallery" : "Yukiri - Custom Title Bar";
	config.m_Width = 1040;
	config.m_Height = 740;
	config.m_Decoration = decoration;
	config.m_Theme = theme;
	config.m_Visible = visible;
	auto window = application.CreateWindow(config);
	if (!window) return std::unexpected(window.error());
	(*window)->SetContent(std::make_unique<Gallery>(application, **window));
	return *window;
}

Gallery::Gallery(Application& application, Window& window) : m_Application(application), m_Window(window)
{
	auto root = std::make_unique<StackPanel>(Orientation::Vertical, 20.0f);
	root->SetPadding(Insets(24));
	auto& heading = root->AddChild<StackPanel>(Orientation::Vertical, 4.0f);
	Heading(heading, "Yukiri", 32);
	Heading(heading, "Modern desktop UI, thoughtfully simple.  /  简洁的现代桌面界面", 13);
	auto& actions = root->AddChild<ResponsivePanel>(230.0f, 4, 8.0f);
	auto& themeGroup = actions.AddChild<Grid>();
	themeGroup.SetColumns({GridLength::Star(), GridLength::Pixels(72)});
	auto& theme = themeGroup.AddChild<ComboBox>(std::vector<std::string>{"System / 跟随系统", "Light / 浅色", "Dark / 深色"});
	m_ThemeSelector = &theme;
	auto& themeButton = themeGroup.AddChild<Button>("深 / 浅");
	m_ThemeButton = &themeButton;
	auto buttonCell = themeGroup.SetCell(themeButton, 0, 1);
	(void)buttonCell;
	auto themeIndex = theme.SetSelectedIndex(static_cast<int>(window.GetConfig().m_Theme));
	(void)themeIndex;
	auto& newWindow = actions.AddChild<Button>("New native window / 新窗口");
	auto& customWindow = actions.AddChild<Button>("Custom title bar / 自绘标题栏");
	auto& reset = actions.AddChild<Button>("Reset controls / 重置");
	m_Tabs = &root->AddChild<TabView>();
	auto controls = std::make_unique<ResponsivePanel>(340.0f, 2, 16.0f);
	auto& actionCard = controls->AddChild<Card>();
	Heading(actionCard, "Actions / 按钮", 18);
	Heading(actionCard, "Click, focus, and keyboard activation", 12);
	auto& primary = actionCard.AddChild<Button>("Primary action / 主要操作");
	primary.SetAccent(true);
	auto& secondary = actionCard.AddChild<Button>("Secondary action / 次要操作");
	auto& disabled = actionCard.AddChild<Button>("Disabled / 已禁用");
	disabled.SetEnabled(false);
	auto& actionHint = Heading(actionCard, "Tab to focus · Enter / Space to activate", 12);
	(void)actionHint;
	auto& selectionCard = controls->AddChild<Card>();
	Heading(selectionCard, "Selection / 选择", 18);
	m_Check = &selectionCard.AddChild<CheckBox>("Enable notifications / 启用通知", true);
	auto& toggle = selectionCard.AddChild<ToggleSwitch>("Preview option / 预览选项", true);
	m_Toggle = &toggle;
	auto& radioA = selectionCard.AddChild<RadioButton>("Option A / 选项 A", "Preview");
	auto& radioB = selectionCard.AddChild<RadioButton>("Option B / 选项 B", "Preview");
	radioA.SetChecked(true);
	auto& inputCard = controls->AddChild<Card>();
	Heading(inputCard, "Inputs / 输入", 18);
	Heading(inputCard, "UTF-8 text, IME, selection and values", 12);
	m_Editor = &inputCard.AddChild<TextBox>();
	m_Editor->SetPlaceholder("Type English or 中文…");
	m_Choice = &inputCard.AddChild<ComboBox>(std::vector<std::string>{"Balanced / 平衡", "Performance / 性能", "Quiet / 安静", "Custom / 自定义"});
	m_Slider = &inputCard.AddChild<Slider>(0.64f);
	m_Progress = &inputCard.AddChild<ProgressBar>(0.64f);
	auto& percentage = Heading(inputCard, "Progress 64% / 进度", 13);
	auto& layoutCard = controls->AddChild<Card>();
	Heading(layoutCard, "Layout / 布局", 18);
	auto& form = layoutCard.AddChild<Grid>();
	form.SetColumns({GridLength::Pixels(90), GridLength::Star()});
	form.SetRows({GridLength::Auto()});
	form.SetSpacing(10, 0);
	auto& name = form.AddChild<Label>("Name / 名称");
	name.SetAlignment(Alignment::Start, Alignment::Center);
	auto& nameValue = form.AddChild<TextBox>("Yukiri");
	auto cell = form.SetCell(nameValue, 0, 1);
	(void)cell;
	Heading(layoutCard, "Nested scrolling / 嵌套滚动", 12);
	m_InnerScroll = &layoutCard.AddChild<ScrollView>();
	m_InnerScroll->SetHeight(102);
	auto rows = std::make_unique<StackPanel>(Orientation::Vertical, 4.0f);
	for (int i = 1; i <= 12; ++i)
	{
		auto& row = rows->AddChild<Button>(std::format("Scrollable item {:02} / 项目", i));
		row.SetHeight(28);
		Keep(row.m_Clicked.Connect([this, i] { m_Status->SetText(std::format("Selected scroll item {}", i)); }));
	}
	m_InnerScroll->SetContent(std::move(rows));
	auto& overlay = layoutCard.AddChild<Overlay>();
	overlay.SetHeight(42);
	WidgetStyle overlayStyle;
	overlayStyle.m_Background = Color::FromHex(0x3674EA, 0.10f);
	overlay.SetStyle(overlayStyle);
	auto& overlayLabel = overlay.AddChild<Label>("Overlay alignment / 叠放对齐");
	overlayLabel.SetAlignment(Alignment::Center, Alignment::Center);
	m_Tabs->AddTab("Controls / 控件", std::move(controls));
	auto textPage = std::make_unique<ResponsivePanel>(340.0f, 2, 16.0f);
	auto& textCard = textPage->AddChild<Card>();
	Heading(textCard, "Typography / 文字", 18);
	auto& sample = Heading(textCard, "Yukiri, 你好，世界。\n0123456789  AV ffi\nText shaping and font fallback.", 18);
	sample.SetWrap(true);
	auto& editor = textCard.AddChild<TextBox>("Select, copy, paste / 选择与编辑");
	(void)editor;
	auto& readOnly = textCard.AddChild<TextBox>("Read-only / 只读文本");
	readOnly.SetReadOnly(true);
	auto& canvasCard = textPage->AddChild<Card>();
	Heading(canvasCard, "Custom drawing / 自定义绘制", 18);
	canvasCard.AddChild<CanvasSample>();
	Heading(canvasCard, "Clip stacks, linear blending, and sRGB", 12);
	m_Tabs->AddTab("Text & Canvas / 文字与绘制", std::move(textPage));
	auto about = std::make_unique<Card>();
	Heading(*about, "Yukiri 0.1.0", 24);
	auto& description = Heading(*about, "C++23 · Retained widgets · Vulkan 1.4\nSDL3 · FreeType · HarfBuzz\n\nWindows preview. Linux support is planned.\nUse Tab / Shift+Tab to navigate and arrow keys to adjust controls.", 14);
	description.SetWrap(true);
	m_Tabs->AddTab("About / 关于", std::move(about));
	m_Status = &Heading(*root, "Ready / 就绪 — interact with any control to see its state.", 12);
	SetContent(std::move(root));
	Keep(primary.m_Clicked.Connect([this] { m_Status->SetText(std::format("Primary action clicked {} time(s).", ++m_ClickCount)); }));
	Keep(secondary.m_Clicked.Connect([this] { m_Status->SetText("Secondary action activated / 次要操作已触发"); }));
	Keep(m_Check->m_CheckedChanged.Connect([this](bool checked) { m_Status->SetText(checked ? "Notifications enabled / 通知已启用" : "Notifications disabled / 通知已关闭"); }));
	Keep(toggle.m_CheckedChanged.Connect([this](bool checked) { m_Status->SetText(checked ? "Preview on / 预览已开启" : "Preview off / 预览已关闭"); }));
	Keep(radioA.m_CheckedChanged.Connect([this](bool checked) { if (checked) m_Status->SetText("Option A selected / 已选择 A"); }));
	Keep(radioB.m_CheckedChanged.Connect([this](bool checked) { if (checked) m_Status->SetText("Option B selected / 已选择 B"); }));
	Keep(m_Slider->m_ValueChanged.Connect([this, &percentage](float value)
	{
		m_Progress->SetValue(value);
		percentage.SetText(std::format("Progress {:.0f}% / 进度", value * 100));
	}));
	Keep(m_Choice->m_SelectionChanged.Connect([this](int) { m_Status->SetText("Selected: " + std::string(m_Choice->GetSelectedText())); }));
	Keep(m_Editor->m_TextChanged.Connect([this](std::string text) { m_Status->SetText("Text: " + text); }));
	Keep(m_Editor->m_Submitted.Connect([this] { m_Status->SetText("Text submitted / 文本已提交: " + m_Editor->GetText()); }));
	Keep(theme.m_SelectionChanged.Connect([this](int index) { ApplyTheme(static_cast<ThemeMode>(index)); }));
	Keep(themeButton.m_Clicked.Connect([this] { ToggleTheme(); }));
	Keep(m_Tabs->m_SelectionChanged.Connect([this](std::size_t) { SetOffset({0, 0}); }));
	Keep(newWindow.m_Clicked.Connect([this]
	{
		auto opened = OpenGallery(m_Application, WindowDecoration::Native, m_Window.GetConfig().m_Theme);
		if (!opened) m_Status->SetText(opened.error().m_Message);
	}));
	Keep(customWindow.m_Clicked.Connect([this]
	{
		auto opened = OpenGallery(m_Application, WindowDecoration::Custom, m_Window.GetConfig().m_Theme);
		if (!opened) m_Status->SetText(opened.error().m_Message);
	}));
	Keep(reset.m_Clicked.Connect([this]
	{
		m_Editor->SetText("");
		m_Slider->SetValue(0.64f);
		m_Check->SetChecked(true);
		auto selected = m_Choice->SetSelectedIndex(0);
		(void)selected;
		m_Status->SetText("Controls reset / 控件已重置");
	}));
}

void Gallery::ApplyTheme(ThemeMode mode)
{
	if (m_Window.GetConfig().m_Theme != mode) m_Window.SetTheme(mode);
	auto selected = m_ThemeSelector->SetSelectedIndex(static_cast<int>(mode));
	if (!selected) m_Status->SetText(selected.error().m_Message);
}
void Gallery::ToggleTheme()
{
	// Resolve the displayed palette so System mode toggles to the opposite appearance.
	const auto color = m_Window.GetTheme().m_Background;
	const bool dark = color.m_R * 0.2126f + color.m_G * 0.7152f + color.m_B * 0.0722f < 0.5f;
	ApplyTheme(dark ? ThemeMode::Light : ThemeMode::Dark);
}
bool Gallery::IsThemeSynchronized() const
{
	return m_ThemeSelector->GetSelectedIndex() == static_cast<int>(m_Window.GetConfig().m_Theme);
}
void Gallery::Exercise(int frame)
{
	m_Slider->SetValue(static_cast<float>(frame % 100) / 100);
	if (frame == 2 || frame == 3 || frame == 46)
	{
		KeyEvent enter{Key::Enter};
		m_ThemeButton->OnKeyDown(enter);
	}
	if (frame == 4) { auto selected = m_ThemeSelector->SetSelectedIndex(0); (void)selected; }
	if (frame == 8) m_Editor->SetText("Yukiri 中文 smoke");
	if (frame == 11 || frame == 14) { m_Check->SetChecked(frame == 14); m_Toggle->SetChecked(frame == 14); }
	if (frame == 12 || frame == 13)
	{
		const auto bounds = m_InnerScroll->GetBounds();
		PointerEvent wheel{{bounds.m_X + 10, bounds.m_Y + 10}, PointerButton::None, {}, 0, frame == 12 ? -2.0f : 1.0f};
		m_InnerScroll->OnWheel(wheel);
	}
	if (frame >= 16 && frame <= 19)
	{
		const auto bounds = m_Choice->GetBounds();
		PointerEvent click{{bounds.m_X + 10, bounds.m_Y + 10}, PointerButton::Left};
		m_Choice->OnPointerDown(click);
	}
	if (frame == 20) { auto selected = m_Tabs->SetSelectedIndex(1); (void)selected; }
	if (frame == 40) { auto selected = m_Tabs->SetSelectedIndex(0); (void)selected; }
	if (frame == 45) ApplyTheme(ThemeMode::Dark);
}
}
