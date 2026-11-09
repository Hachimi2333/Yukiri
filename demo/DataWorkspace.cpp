#include "DataWorkspace.hpp"
#include <format>
#include <semaphore>
#include <chrono>

namespace YukiriDemo
{
using namespace Yukiri;
namespace
{
ItemModel::Cell MakeCell(std::string text, bool checked = false, bool showCheck = false)
{
	ItemModel::Cell cell; cell.m_Text = std::move(text); cell.m_Editable = true;
	if (showCheck) cell.m_Checked = checked;
	return cell;
}
void Name(Widget& widget, std::string name)
{
	AccessibilityInfo info; info.m_Name = std::move(name); widget.SetAccessibilityInfo(std::move(info));
}
}
Result<Window*> OpenData(Application& application, WindowDecoration decoration, ThemeMode theme)
{
	WindowConfig config; config.m_Title = "Yukiri - Data Manager";
	config.m_Width = 1200; config.m_Height = 800; config.m_Decoration = decoration; config.m_Theme = theme;
	auto window = application.CreateWindow(config);
	if (!window) return std::unexpected(window.error());
	(*window)->SetContent(std::make_unique<DataWorkspace>(**window));
	return *window;
}
DataWorkspace::DataWorkspace(Window& window) : m_Window(window),
	m_Model(std::make_shared<StandardItemModel>(std::vector<std::string>{"Name / 名称", "Status / 状态", "Notes / 备注"})),
	m_Selection(std::make_shared<SelectionModel>(m_Model))
{
	SetRows({GridLength::Auto(), GridLength::Auto(), GridLength::Star(), GridLength::Auto()}); SetSpacing(0, 0);
	m_Selection->SetMode(SelectionModel::Mode::Multiple);
	auto command = [this](std::string text, Key key, auto action)
	{
		auto result = std::make_shared<Command>(std::move(text));
		if (key != Key::Unknown) result->SetShortcut(KeyGesture{key, {.m_Control = true}});
		Keep(result->m_Executed.Connect(std::move(action))); Keep(m_Window.ConnectCommand(result)); return result;
	};
	m_Add = command("Add row / 添加", Key::N, [this] { Add(); });
	m_Remove = command("Delete selected / 删除选中", Key::Delete, [this] { Remove(); });
	m_Edit = command("Edit cell / 编辑单元格", Key::Unknown, [this]
	{
		const auto result = m_Table->BeginEdit(m_Selection->GetCurrent(), m_Table->GetCurrentColumn());
		if (!result) m_Status->SetText(result.error().m_Message);
	});
	m_Load = command("Load rows / 后台加载", Key::L, [this] { Load(); });
	m_Cancel = command("Cancel load / 取消加载", Key::Unknown, [this] { Cancel(); });
	auto sort = command("Sort by name / 按名称排序", Key::Unknown, [this] { const auto result = m_Table->Sort(0); if (!result) m_Status->SetText(result.error().m_Message); });
	auto theme = command("Light / dark / 深浅切换", Key::Unknown, [this] { m_Window.SetTheme(m_Window.GetTheme().m_Background.m_R < 0.5f ? ThemeMode::Light : ThemeMode::Dark); });
	using Item = ContextMenu::Item;
	auto& menu = AddChild<MenuBar>();
	menu.SetItems({Item::Submenu("Data / 数据", {Item::Action(m_Add), Item::Action(m_Remove), Item::Action(m_Edit), Item::Divider(), Item::Action(m_Load), Item::Action(m_Cancel)}),
		Item::Submenu("View / 视图", {Item::Action(sort), Item::Action(theme)})});
	auto& toolbar = AddChild<ToolBar>(); (void)SetCell(toolbar, 1, 0);
	for (const auto& action : {m_Add, m_Remove, m_Edit, sort}) { auto& button = toolbar.AddChild<Button>(); button.SetCommand(action); button.SetToolTip(action->GetText()); }
	m_Count = &toolbar.AddChild<NumberBox>(100000.0);
	m_Count->SetWidth(130); (void)m_Count->SetRange(1, 100000); (void)m_Count->SetStep(1000); (void)m_Count->SetFormat(NumberBox::Format::Fixed, 0);
	Name(*m_Count, "Number of rows to load / 加载行数");
	for (const auto& action : {m_Load, m_Cancel}) { auto& button = toolbar.AddChild<Button>(); button.SetCommand(action); }
	m_Progress = &toolbar.AddChild<ProgressBar>(); m_Progress->SetWidth(140); m_Progress->SetHeight(8); Name(*m_Progress, "Background load progress / 后台加载进度");
	auto& split = AddChild<SplitPanel>(); (void)SetCell(split, 2, 0); split.SetMargin(Insets(10)); split.SetRatio(0.28f); split.SetMinimumSizes(160, 240);
	auto left = std::make_unique<Grid>(); left->SetRows({GridLength::Auto(), GridLength::Star()});
	left->AddChild<Label>("List / 列表 — shared selection / 共享选择");
	m_List = &left->AddChild<ListView>(); (void)left->SetCell(*m_List, 1, 0); Name(*m_List, "Records list / 记录列表");
	auto right = std::make_unique<SplitPanel>(Orientation::Vertical); right->SetRatio(0.34f); right->SetMinimumSizes(100, 160);
	auto treePane = std::make_unique<Grid>(); treePane->SetRows({GridLength::Auto(), GridLength::Star()});
	treePane->AddChild<Label>("Tree / 树 — expand sample rows / 展开示例行");
	m_Tree = &treePane->AddChild<TreeView>(); (void)treePane->SetCell(*m_Tree, 1, 0); Name(*m_Tree, "Records hierarchy / 记录层级");
	auto tablePane = std::make_unique<Grid>(); tablePane->SetRows({GridLength::Auto(), GridLength::Star()});
	tablePane->AddChild<Label>("Table / 表格 — F2 edit · Ctrl/Shift selection · click header to sort");
	m_Table = &tablePane->AddChild<TableView>(); (void)tablePane->SetCell(*m_Table, 1, 0); Name(*m_Table, "Editable records / 可编辑记录");
	right->SetContent(std::move(treePane), std::move(tablePane)); split.SetContent(std::move(left), std::move(right));
	for (auto* view : {m_List, static_cast<ListView*>(m_Tree), static_cast<ListView*>(m_Table)})
	{
		view->SetModel(m_Model); (void)view->SetSelectionModel(m_Selection);
		ContextMenu::Bind(*view, {Item::Action(m_Add), Item::Action(m_Edit), Item::Action(m_Remove), Item::Divider(), Item::Action(sort)});
		Keep(view->m_ItemActivated.Connect([this](ItemId item) { auto edited = m_Table->BeginEdit(item, 0); if (!edited) m_Status->SetText(edited.error().m_Message); }));
	}
	(void)m_Table->SetColumnWidth(0, 260); (void)m_Table->SetColumnWidth(1, 150); (void)m_Table->SetColumnWidth(2, 320);
	auto& status = AddChild<StatusBar>(); (void)SetCell(status, 3, 0); m_Status = &status.AddChild<Label>();
	Keep(m_Selection->m_Changed.Connect([this] { Refresh(); }));
	Keep(m_Model->m_Changed.Connect([this](const ItemModel::Change&) { Refresh(); }));
	for (int index = 0; index < 12; ++index)
	{
		auto row = m_Model->AppendRow({MakeCell(std::format("Record {:03} / 记录", index + 1), index % 2 == 0, true), MakeCell("Ready / 就绪"), MakeCell("Double-click or F2 to edit / 双击或 F2 编辑")});
		if (row && index < 3) { (void)m_Model->AppendRow({MakeCell("Child / 子项"), MakeCell("Detail / 详情"), MakeCell("The same model is used by all three views.")}, *row); m_Tree->SetExpanded(*row, true); }
	}
	Refresh();
}
DataWorkspace::~DataWorkspace() { m_Lifetime.reset(); m_Worker.request_stop(); }
void DataWorkspace::Refresh()
{
	m_Add->SetEnabled(!m_Loading); m_Remove->SetEnabled(!m_Loading && !m_Selection->GetSelected().empty());
	m_Edit->SetEnabled(!m_Loading && m_Selection->GetCurrent() != 0); m_Load->SetEnabled(!m_Loading); m_Cancel->SetEnabled(m_Loading); m_Count->SetEnabled(!m_Loading);
	m_Status->SetText(m_Loading ? std::format("Loading {} / {} · {} selected / 已选", m_Loaded, m_Total, m_Selection->GetSelected().size()) :
		std::format("{} root rows / 根行 · {} selected / 已选 · Ctrl+L load · Ctrl+N add · Ctrl+Delete remove", m_Model->GetRowCount(), m_Selection->GetSelected().size()));
}
void DataWorkspace::Add()
{
	auto row = m_Model->AppendRow({MakeCell("New record / 新记录", false, true), MakeCell("Ready"), MakeCell("")});
	if (!row) { m_Status->SetText(row.error().m_Message); return; }
	m_Selection->Select(*row); m_Table->ScrollIntoView(*row); (void)m_Table->BeginEdit(*row, 0);
}
void DataWorkspace::Remove()
{
	const auto selected = m_Selection->GetSelected();
	for (const auto item : selected) if (m_Model->Contains(item)) { auto removed = m_Model->Remove(item); if (!removed) m_Status->SetText(removed.error().m_Message); }
}
void DataWorkspace::Cancel()
{
	++m_Generation; m_Worker.request_stop(); m_Loading = false; Refresh();
	m_Status->SetText(std::format("Load canceled / 加载已取消 · {} rows retained / 已保留行", m_Model->GetRowCount()));
}
void DataWorkspace::AcceptBatch(std::uint64_t generation, std::vector<std::vector<ItemModel::Cell>> rows, std::size_t completed, std::size_t total)
{
	if (generation != m_Generation || !m_Loading) return;
	auto inserted = m_Model->AppendRows(std::move(rows));
	if (!inserted) { Cancel(); m_Status->SetText(inserted.error().m_Message); return; }
	m_Loaded = completed; m_Total = total; m_Progress->SetValue(static_cast<float>(completed) / static_cast<float>(total));
	if (completed == total) m_Loading = false;
	Refresh();
}
void DataWorkspace::Load()
{
	if (!m_Count->Commit()) { m_Status->SetText("Enter a valid row count / 请输入有效行数"); return; }
	Cancel();
	m_Worker = {};
	const auto count = static_cast<std::size_t>(m_Count->GetValue());
	m_Loading = true; m_Loaded = 0; m_Total = count; m_Progress->SetValue(0); m_Model->Clear(); Refresh();
	const auto generation = ++m_Generation;
	m_Worker = std::jthread([dispatcher = m_Window.GetDispatcher(), weak = std::weak_ptr(m_Lifetime), generation, count](std::stop_token stop)
	{
		auto acknowledgment = std::make_shared<std::binary_semaphore>(0);
		for (std::size_t offset = 0; offset < count && !stop.stop_requested();)
		{
			std::vector<std::vector<ItemModel::Cell>> rows;
			const auto end = std::min(count, offset + 1000); rows.reserve(end - offset);
			for (; offset < end; ++offset) rows.push_back({MakeCell(std::format("Record {:06} / 记录", offset + 1), offset % 2 == 0, true), MakeCell(offset % 3 == 0 ? "Active / 活跃" : "Ready / 就绪"), MakeCell(std::format("Background-generated item {}", offset + 1))});
			if (stop.stop_requested()) break;
			auto posted = dispatcher.Post([weak, acknowledgment, generation, rows = std::move(rows), completed = offset, count]() mutable
			{
				if (const auto lifetime = weak.lock()) lifetime->m_Owner->AcceptBatch(generation, std::move(rows), completed, count);
				acknowledgment->release();
			});
			if (!posted) break;
			// Bound queued data to one batch; stop requests also release a closing window.
			while (!acknowledgment->try_acquire_for(std::chrono::milliseconds(20))) if (stop.stop_requested()) return;
		}
	});
}
}
