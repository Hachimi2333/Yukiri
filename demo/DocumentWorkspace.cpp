#include "DocumentWorkspace.hpp"
#include <format>

namespace YukiriDemo
{
using namespace Yukiri;
namespace
{
void Name(Widget& widget, std::string text)
{
	AccessibilityInfo info; info.m_Name = std::move(text); widget.SetAccessibilityInfo(std::move(info));
}
}
Result<Window*> OpenDocument(Application& application, WindowDecoration decoration, ThemeMode theme)
{
	WindowConfig config;
	config.m_Title = "Yukiri - Document Editor";
	config.m_Width = 1080; config.m_Height = 760; config.m_Decoration = decoration; config.m_Theme = theme;
	auto window = application.CreateWindow(config);
	if (!window) return std::unexpected(window.error());
	(*window)->SetContent(std::make_unique<DocumentWorkspace>(**window));
	return *window;
}
std::shared_ptr<Command> DocumentWorkspace::MakeCommand(std::string text, Key key, bool shift, Action action)
{
	auto command = std::make_shared<Command>(std::move(text));
	if (key != Key::Unknown) command->SetShortcut(KeyGesture{key, {.m_Shift = shift, .m_Control = true}});
	Keep(command->m_Executed.Connect(std::move(action)));
	Keep(m_Window.ConnectCommand(command));
	return command;
}
DocumentWorkspace::DocumentWorkspace(Window& window) : m_Window(window)
{
	SetRows({GridLength::Auto(), GridLength::Auto(), GridLength::Auto(), GridLength::Star(), GridLength::Auto()});
	SetSpacing(0, 0);
	m_New = MakeCommand("New / 新建", Key::N, false, [this] { GuardUnsaved([this] { m_Document.New(); m_Editor->SetText(""); Refresh(); }); });
	m_Open = MakeCommand("Open… / 打开", Key::O, false, [this] { GuardUnsaved([this] { ChooseOpen(); }); });
	m_Save = MakeCommand("Save / 保存", Key::S, false, [this] { Save(false); });
	m_SaveAs = MakeCommand("Save as… / 另存为", Key::S, true, [this] { Save(true); });
	m_Undo = MakeCommand("Undo / 撤销", Key::Z, false, [this] { m_Editor->Undo(); });
	m_Redo = MakeCommand("Redo / 重做", Key::Y, false, [this] { m_Editor->Redo(); });
	m_FindCommand = MakeCommand("Find / Replace / 查找替换", Key::F, false, [this] { m_FindPanel->SetVisible(true); m_Find->RequestFocus(); m_Find->SelectAll(); });
	auto close = MakeCommand("Close / 关闭", Key::W, false, [this] { m_Window.Close(); });
	auto selectAll = MakeCommand("Select all / 全选", Key::Unknown, false, [this] { m_Editor->SelectAll(); m_Editor->RequestFocus(); });
	auto cut = MakeCommand("Cut / 剪切", Key::Unknown, false, [this] { auto result = m_Editor->Cut(); if (!result) Error(result.error().m_Message); });
	auto copy = MakeCommand("Copy / 复制", Key::Unknown, false, [this] { auto result = m_Editor->Copy(); if (!result) Error(result.error().m_Message); });
	auto paste = MakeCommand("Paste / 粘贴", Key::Unknown, false, [this] { auto result = m_Editor->Paste(); if (!result) Error(result.error().m_Message); });
	auto wrap = MakeCommand("Word wrap / 自动换行", Key::Unknown, false, [this] { m_Editor->SetWrap(!m_Editor->GetWrap()); });
	wrap->SetCheckable(true); wrap->SetChecked(true);
	Keep(wrap->m_Executed.Connect([this, weak = std::weak_ptr(wrap)] { if (auto command = weak.lock()) command->SetChecked(m_Editor->GetWrap()); }));
	auto theme = MakeCommand("Light / dark / 深浅切换", Key::Unknown, false, [this]
	{
		m_Window.SetTheme(m_Window.GetTheme().m_Background.m_R < 0.5f ? ThemeMode::Light : ThemeMode::Dark);
	});
	using Item = ContextMenu::Item;
	auto& menu = AddChild<MenuBar>();
	menu.SetItems({Item::Submenu("File / 文件", {Item::Action(m_New), Item::Action(m_Open), Item::Action(m_Save), Item::Action(m_SaveAs), Item::Divider(), Item::Action(close)}),
		Item::Submenu("Edit / 编辑", {Item::Action(m_Undo), Item::Action(m_Redo), Item::Divider(), Item::Action(cut), Item::Action(copy), Item::Action(paste), Item::Action(selectAll), Item::Action(m_FindCommand)}),
		Item::Submenu("View / 视图", {Item::Action(wrap), Item::Action(theme)})});
	auto& toolbar = AddChild<ToolBar>();
	(void)SetCell(toolbar, 1, 0);
	for (const auto& command : {m_New, m_Open, m_Save, m_Undo, m_Redo, m_FindCommand})
	{
		auto& button = toolbar.AddChild<Button>(); button.SetCommand(command); button.SetToolTip(command->GetText());
	}
	m_FindPanel = &AddChild<WrapPanel>();
	m_FindPanel->SetPadding(Insets(8)); m_FindPanel->SetSpacing(6, 6);
	(void)SetCell(*m_FindPanel, 2, 0);
	m_Find = &m_FindPanel->AddChild<TextBox>(); m_Find->SetWidth(190); m_Find->SetPlaceholder("Find / 查找"); Name(*m_Find, "Find text / 查找文本");
	m_Replacement = &m_FindPanel->AddChild<TextBox>(); m_Replacement->SetWidth(190); m_Replacement->SetPlaceholder("Replace with / 替换为"); Name(*m_Replacement, "Replacement / 替换文本");
	m_MatchCase = &m_FindPanel->AddChild<CheckBox>("Match case / 区分大小写");
	for (const auto& [title, mode] : {std::pair{"Next / 下一个", 0}, {"Replace / 替换", 1}, {"All / 全部替换", 2}})
	{
		auto& button = m_FindPanel->AddChild<Button>(title);
		Keep(button.m_Clicked.Connect([this, mode] { Find(mode != 0, mode == 2); }));
	}
	auto& hideFind = m_FindPanel->AddChild<Button>("Hide / 收起");
	Keep(hideFind.m_Clicked.Connect([this] { m_FindPanel->SetVisible(false); m_Editor->RequestFocus(); }));
	m_FindPanel->SetVisible(false);
	m_Editor = &AddChild<TextArea>();
	m_Editor->SetMargin(Insets(10)); m_Editor->SetWrap(true); Name(*m_Editor, "Document text / 文档内容");
	m_Editor->SetPlaceholder("New UTF-8 document / 新建 UTF-8 文档\nCtrl+O open · Ctrl+S save · Ctrl+F find and replace");
	(void)SetCell(*m_Editor, 3, 0);
	ContextMenu::Bind(*m_Editor, {Item::Action(m_Undo), Item::Action(m_Redo), Item::Divider(), Item::Action(cut), Item::Action(copy), Item::Action(paste), Item::Action(selectAll)});
	auto& status = AddChild<StatusBar>(); (void)SetCell(status, 4, 0);
	m_Progress = &status.AddChild<ProgressBar>(); m_Progress->SetWidth(100); m_Progress->SetHeight(8); m_Progress->SetVisible(false);
	m_Status = &status.AddChild<Label>();
	Keep(m_Editor->m_TextChanged.Connect([this](std::string text) { m_Document.SetText(std::move(text)); Refresh(); }));
	Keep(m_Editor->m_EditStateChanged.Connect([this] { Refresh(); }));
	Keep(m_Window.m_CloseRequested.Connect([this](CloseRequestEvent& event)
	{
		if (m_AllowClose) return;
		if (m_Busy || m_Confirmation) { event.m_Cancel = true; return; }
		if (!m_Document.IsDirty()) return;
		event.m_Cancel = true;
		GuardUnsaved([this] { m_AllowClose = true; m_Window.Close(); });
	}));
	Keep(m_Window.m_FilesDropped.Connect([this](std::vector<std::filesystem::path> paths)
	{
		if (!paths.empty()) GuardUnsaved([this, path = std::move(paths.front())] { Read(path); });
	}));
	Refresh();
}
DocumentWorkspace::~DocumentWorkspace() { m_Lifetime.reset(); m_Worker.request_stop(); }
void DocumentWorkspace::Error(std::string text)
{
	m_Status->SetText(text);
	auto dialog = MessageDialog::Show(m_Window, "Document / 文档", std::move(text));
	(void)dialog;
}
void DocumentWorkspace::Refresh()
{
	m_Save->SetEnabled(!m_Busy); m_SaveAs->SetEnabled(!m_Busy); m_New->SetEnabled(!m_Busy); m_Open->SetEnabled(!m_Busy);
	m_Undo->SetEnabled(m_Editor->CanUndo()); m_Redo->SetEnabled(m_Editor->CanRedo());
	const auto title = m_Document.GetPath().empty() ? "Untitled / 未命名" : PathText(m_Document.GetPath().filename());
	m_Window.SetTitle(std::string(m_Document.IsDirty() ? "* " : "") + title + " - Yukiri Editor");
	m_Progress->SetVisible(m_Busy); m_Progress->SetIndeterminate(m_Busy);
	if (!m_Busy) m_Status->SetText(std::format("{} · {} UTF-8 bytes · {}", title, m_Document.GetText().size(), m_Document.IsDirty() ? "Modified / 未保存" : "Saved / 已保存"));
}
void DocumentWorkspace::GuardUnsaved(Action action)
{
	if (m_Busy || m_Confirmation) return;
	if (!m_Document.IsDirty()) { action(); return; }
	m_Confirmation = true;
	auto result = MessageDialog::Show(m_Window, "Unsaved document / 未保存文档", "Save your changes before continuing? / 是否保存更改？",
		MessageDialog::Buttons::SaveDiscardCancel, [weak = std::weak_ptr(m_Lifetime), action = std::move(action)](Dialog::Result choice) mutable
	{
		const auto lifetime = weak.lock(); if (!lifetime) return;
		auto& self = *lifetime->m_Owner;
		self.m_Confirmation = false;
		if (choice == Dialog::Result::Discard) action();
		else if (choice == Dialog::Result::Save) self.Save(false, std::move(action));
	});
	if (!result) { m_Confirmation = false; m_Status->SetText(result.error().m_Message); }
}
void DocumentWorkspace::ChooseOpen()
{
	m_Busy = true; m_Editor->SetReadOnly(true); Refresh(); m_Status->SetText("Choose a UTF-8 file / 选择 UTF-8 文件");
	FileDialog::Config config; config.m_Title = "Open UTF-8 document"; config.m_DefaultPath = m_Document.GetPath();
	config.m_Filters = {{"Text documents", "*.txt;*.md;*.log"}, {"All files", "*.*"}};
	auto result = FileDialog::Open(m_Window, std::move(config), [weak = std::weak_ptr(m_Lifetime)](Result<std::vector<std::filesystem::path>> paths)
	{
		const auto lifetime = weak.lock(); if (!lifetime) return;
		auto& self = *lifetime->m_Owner; self.m_Busy = false; self.m_Editor->SetReadOnly(false);
		if (!paths) { self.Refresh(); self.Error(paths.error().m_Message); }
		else if (paths->empty()) self.Refresh();
		else self.Read(std::move(paths->front()));
	});
	if (!result) { m_Busy = false; m_Editor->SetReadOnly(false); Refresh(); Error(result.error().m_Message); }
}
void DocumentWorkspace::Read(std::filesystem::path path)
{
	if (m_Busy) return;
	m_Busy = true; m_Editor->SetReadOnly(true); Refresh(); m_Status->SetText("Reading document… / 正在读取");
	m_Worker = std::jthread([path = std::move(path), dispatcher = m_Window.GetDispatcher(), weak = std::weak_ptr(m_Lifetime)](std::stop_token stop)
	{
		auto result = ReadDocument(path, stop);
		if (stop.stop_requested()) return;
		auto posted = dispatcher.Post(weak, [path, result = std::move(result)](Lifetime& alive) mutable
		{
			auto& self = *alive.m_Owner; self.m_Busy = false; self.m_Editor->SetReadOnly(false);
			if (!result) { self.Refresh(); self.Error(result.error().m_Message); return; }
			self.m_Document.Load(path, std::move(*result)); self.m_Editor->SetText(self.m_Document.GetText()); self.Refresh(); self.m_Editor->RequestFocus();
		});
		(void)posted;
	});
}
void DocumentWorkspace::Save(bool saveAs, Action completed)
{
	if (m_Busy) return;
	if (!saveAs && !m_Document.GetPath().empty()) { Write(m_Document.GetPath(), std::move(completed)); return; }
	m_Busy = true; Refresh(); m_Status->SetText("Choose a destination / 选择保存位置");
	FileDialog::Config config; config.m_Title = "Save UTF-8 document"; config.m_DefaultPath = m_Document.GetPath(); config.m_DefaultExtension = "txt";
	config.m_Filters = {{"Text document", "*.txt"}, {"Markdown", "*.md"}, {"All files", "*.*"}};
	auto result = FileDialog::Save(m_Window, std::move(config), [weak = std::weak_ptr(m_Lifetime), completed = std::move(completed)](Result<std::vector<std::filesystem::path>> paths) mutable
	{
		const auto lifetime = weak.lock(); if (!lifetime) return;
		auto& self = *lifetime->m_Owner; self.m_Busy = false;
		if (!paths) { self.Refresh(); self.Error(paths.error().m_Message); }
		else if (paths->empty()) self.Refresh();
		else self.Write(std::move(paths->front()), std::move(completed));
	});
	if (!result) { m_Busy = false; Refresh(); Error(result.error().m_Message); }
}
void DocumentWorkspace::Write(std::filesystem::path path, Action completed)
{
	m_Busy = true; Refresh(); m_Status->SetText("Saving document… / 正在保存");
	m_Worker = std::jthread([path = std::move(path), text = m_Document.GetText(), dispatcher = m_Window.GetDispatcher(),
		weak = std::weak_ptr(m_Lifetime), completed = std::move(completed)](std::stop_token stop) mutable
	{
		auto result = WriteDocument(path, text, stop);
		if (stop.stop_requested()) return;
		auto posted = dispatcher.Post(weak, [path, text = std::move(text), result = std::move(result), completed = std::move(completed)](Lifetime& alive) mutable
		{
			auto& self = *alive.m_Owner; self.m_Busy = false;
			if (!result) { self.Refresh(); self.Error(result.error().m_Message); return; }
			self.m_Document.MarkSaved(path, std::move(text)); self.Refresh();
			// A save snapshot must not discard edits made while the worker was writing.
			if (completed) self.GuardUnsaved(std::move(completed));
		});
		(void)posted;
	});
}
void DocumentWorkspace::Find(bool replace, bool all)
{
	const auto pattern = m_Find->GetText();
	if (pattern.empty()) { m_Status->SetText("Enter a search term / 请输入查找文本"); return; }
	const bool matchCase = m_MatchCase->IsChecked();
	if (all) m_Status->SetText(std::format("Replaced {} match(es) / 已替换", m_Editor->ReplaceAll(pattern, m_Replacement->GetText(), matchCase)));
	else
	{
		const bool found = replace ? m_Editor->Replace(pattern, m_Replacement->GetText(), matchCase) : m_Editor->Find(pattern, matchCase);
		m_Status->SetText(found ? "Match selected / 已定位匹配" : "No match / 无匹配");
	}
	m_Editor->RequestFocus();
}
}
