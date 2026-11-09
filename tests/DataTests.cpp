#include <Yukiri/Data/StandardItemModel.hpp>
#include <Yukiri/Data/SelectionModel.hpp>
#include <Yukiri/Controls/ListView.hpp>
#include <Yukiri/Controls/TreeView.hpp>
#include <Yukiri/Controls/TableView.hpp>
#include <Yukiri/Controls/TextBox.hpp>
#include "UI/WidgetTree.hpp"
#include "Render/CanvasFactory.hpp"
#include "Render/DrawList.hpp"
#include <catch2/catch_test_macros.hpp>

namespace
{
Yukiri::ItemModel::Cell Editable(std::string text)
{
	Yukiri::ItemModel::Cell cell; cell.m_Text = std::move(text); cell.m_Editable = true; return cell;
}
class LargeModel : public Yukiri::ItemModel
{
public:
	mutable unsigned m_DataCalls = 0;
	mutable unsigned m_StructureCalls = 0;
	std::size_t GetRowCount(Yukiri::ItemId parent) const override { ++m_StructureCalls; return parent ? 0 : 100000; }
	std::size_t GetColumnCount() const override { return 3; }
	Yukiri::ItemId GetChild(Yukiri::ItemId parent, std::size_t row) const override { ++m_StructureCalls; return !parent && row < 100000 ? row + 1 : 0; }
	Yukiri::ItemId GetParent(Yukiri::ItemId) const override { return 0; }
	bool Contains(Yukiri::ItemId item) const override { return item >= 1 && item <= 100000; }
	Cell GetCell(Yukiri::ItemId item, std::size_t) const override { ++m_DataCalls; return Editable("Row " + std::to_string(item)); }
};
}
TEST_CASE("Standard models retain stable IDs through insertion, sorting, moves, and descendant removal", "[data][model]")
{
	auto model = std::make_shared<Yukiri::StandardItemModel>(std::vector<std::string>{"Name", "Value"});
	const auto parent = model->AppendRow({Editable("Parent")}); REQUIRE(parent);
	const auto child = model->AppendRow({Editable("Child")}, *parent); REQUIRE(child);
	const auto other = model->InsertRow(0, {Editable("Alpha")}); REQUIRE(other);
	REQUIRE(model->GetChild(0, 0) == *other);
	REQUIRE_FALSE(model->Move(*parent, *child, 0));
	REQUIRE(model->Sort(0, false));
	REQUIRE(model->GetChild(0, 0) == *parent);
	REQUIRE(model->GetParent(*child) == *parent);
	REQUIRE(model->Move(*child, 0, 1));
	REQUIRE(model->GetParent(*child) == 0);
	REQUIRE(model->SetText(*child, 0, "Renamed"));
	REQUIRE(model->GetCell(*child).m_Text == "Renamed");
	REQUIRE(model->Remove(*parent));
	REQUIRE(model->Contains(*child));
	model->Clear();
	const auto next = model->AppendRow({Editable("New")}); REQUIRE(next);
	REQUIRE(*next > *child);
}
TEST_CASE("Shared selections survive sort and insertion and prune removed or reset model IDs", "[data][selection]")
{
	auto model = std::make_shared<Yukiri::StandardItemModel>();
	auto ids = model->AppendRows({{Editable("C")}, {Editable("A")}, {Editable("B")}}); REQUIRE(ids);
	auto selection = std::make_shared<Yukiri::SelectionModel>(model);
	selection->SetMode(Yukiri::SelectionModel::Mode::Multiple);
	selection->SetSelected({(*ids)[0], (*ids)[2]}, (*ids)[2]);
	REQUIRE(model->Sort(0));
	REQUIRE(model->AppendRow({Editable("D")}));
	REQUIRE(selection->IsSelected((*ids)[0])); REQUIRE(selection->IsSelected((*ids)[2]));
	REQUIRE(model->Remove((*ids)[0]));
	REQUIRE(selection->GetSelected() == std::vector<Yukiri::ItemId>{(*ids)[2]});
	selection->SetMode(Yukiri::SelectionModel::Mode::Single);
	REQUIRE(selection->GetSelected().size() == 1);
	model->Clear();
	REQUIRE(selection->GetSelected().empty()); REQUIRE(selection->GetCurrent() == 0);
}
TEST_CASE("A hundred thousand rows keep widget counts and hot-scroll model access bounded", "[data][virtualization]")
{
	for (int kind = 0; kind < 3; ++kind)
	{
		auto model = std::make_shared<LargeModel>();
		std::unique_ptr<Yukiri::ListView> view;
		if (kind == 0) view = std::make_unique<Yukiri::ListView>();
		else if (kind == 1) view = std::make_unique<Yukiri::TreeView>();
		else view = std::make_unique<Yukiri::TableView>();
		auto* control = view.get();
		control->SetModel(model);
		Yukiri::Detail::WidgetTree tree; tree.SetRoot(std::move(view)); tree.Layout({480, 240});
		REQUIRE(control->GetRowCount() == 100000);
		REQUIRE(control->GetChildren().empty());
		model->m_DataCalls = model->m_StructureCalls = 0;
		for (int frame = 0; frame < 100; ++frame)
		{
			control->SetOffset({0, static_cast<float>(frame) * 10000});
			Yukiri::Detail::DrawList draw;
			auto canvas = Yukiri::Detail::CanvasFactory::Create(draw, {480, 240});
			tree.Paint(canvas);
			REQUIRE(control->GetVisibleRange().second - control->GetVisibleRange().first <= 9);
			REQUIRE(draw.m_Vertices.size() < 1000);
		}
		REQUIRE(model->m_StructureCalls == 0);
		REQUIRE(model->m_DataCalls <= 100 * 9 * 3);
		REQUIRE(control->GetChildren().empty());
	}
}
TEST_CASE("Tree expansion and keyboard navigation reveal descendants without per-row widgets", "[data][tree]")
{
	auto model = std::make_shared<Yukiri::StandardItemModel>();
	auto parent = model->AppendRow({Editable("Parent")}); REQUIRE(parent);
	auto child = model->AppendRow({Editable("Child")}, *parent); REQUIRE(child);
	auto grandchild = model->AppendRow({Editable("Leaf")}, *child); REQUIRE(grandchild);
	auto view = std::make_unique<Yukiri::TreeView>(); auto* control = view.get(); control->SetModel(model);
	Yukiri::Detail::WidgetTree tree; tree.SetRoot(std::move(view)); tree.Layout({300, 180}); tree.SetFocus(control);
	tree.KeyDown({Yukiri::Key::Home}); REQUIRE(control->GetSelectionModel()->GetCurrent() == *parent);
	tree.KeyDown({Yukiri::Key::Right}); REQUIRE(control->GetRowCount() == 2);
	tree.KeyDown({Yukiri::Key::Right}); REQUIRE(control->GetSelectionModel()->GetCurrent() == *child);
	control->ScrollIntoView(*grandchild); REQUIRE(control->GetRowCount() == 3);
	control->GetSelectionModel()->SetCurrent(*grandchild);
	control->SetExpanded(*parent, false); REQUIRE(control->GetRowCount() == 1);
	REQUIRE(control->GetSelectionModel()->GetCurrent() == *parent);
	REQUIRE(control->GetChildren().empty());
}
TEST_CASE("Item views share range selection and preserve selection by ID after sorting", "[data][view]")
{
	auto model = std::make_shared<Yukiri::StandardItemModel>();
	auto ids = model->AppendRows({{Editable("C")}, {Editable("A")}, {Editable("B")}}); REQUIRE(ids);
	auto view = std::make_unique<Yukiri::TableView>(); auto* table = view.get(); table->SetModel(model);
	table->SetSelectionMode(Yukiri::SelectionModel::Mode::Multiple);
	Yukiri::ListView list; list.SetModel(model); REQUIRE(list.SetSelectionModel(table->GetSelectionModel()));
	Yukiri::Detail::WidgetTree tree; tree.SetRoot(std::move(view)); tree.Layout({300, 180}); tree.SetFocus(table);
	tree.KeyDown({Yukiri::Key::Home}); tree.KeyDown({Yukiri::Key::Down, {true}});
	REQUIRE(list.GetSelectionModel()->GetSelected().size() == 2);
	REQUIRE(table->Sort(0));
	REQUIRE(table->GetItemAt(0) == (*ids)[1]);
	REQUIRE(list.GetSelectionModel()->IsSelected((*ids)[0]));
	REQUIRE(table->SetColumnWidth(0, 250)); REQUIRE(table->GetColumnWidth(0) == 250);
	REQUIRE_FALSE(table->SetColumnWidth(9, 100));
	REQUIRE(model->Remove((*ids)[1])); REQUIRE(list.GetSelectionModel()->GetSelected().size() == 1);
}
TEST_CASE("Cell editors preserve rejected input and retire after commit, cancel, or model removal", "[data][editing]")
{
	class Validated : public Yukiri::StandardItemModel
	{
	public:
		Yukiri::Result<void> SetText(Yukiri::ItemId item, std::size_t column, std::string text) override
		{
			if (text.empty()) return std::unexpected(Yukiri::Error{Yukiri::ErrorCode::InvalidArgument, "Name is required."});
			return StandardItemModel::SetText(item, column, std::move(text));
		}
	};
	auto model = std::make_shared<Validated>();
	auto id = model->AppendRow({Editable("Original")}); REQUIRE(id);
	auto view = std::make_unique<Yukiri::TableView>(); auto* table = view.get(); table->SetModel(model);
	Yukiri::Detail::WidgetTree tree; tree.SetRoot(std::move(view)); tree.Layout({300, 180});
	REQUIRE(table->BeginEdit(*id));
	table->GetEditor()->SetText(""); REQUIRE_FALSE(table->CommitEdit()); REQUIRE(table->GetEditor());
	table->GetEditor()->SetText("Renamed"); tree.KeyDown({Yukiri::Key::Enter});
	REQUIRE(model->GetCell(*id).m_Text == "Renamed"); REQUIRE(table->GetEditor() == nullptr); REQUIRE(table->GetChildren().empty());
	REQUIRE(table->BeginEdit(*id)); tree.KeyDown({Yukiri::Key::Escape}); REQUIRE(table->GetEditor() == nullptr);
	REQUIRE(table->BeginEdit(*id)); REQUIRE(model->Remove(*id)); REQUIRE(table->GetEditor() == nullptr);
}
TEST_CASE("Late model notifications do not retain destroyed views", "[data][lifetime]")
{
	auto model = std::make_shared<Yukiri::StandardItemModel>();
	Yukiri::Detail::WidgetTree tree;
	auto view = std::make_unique<Yukiri::ListView>(); view->SetModel(model);
	tree.SetRoot(std::move(view)); tree.SetRoot(nullptr);
	REQUIRE(model->AppendRow({Editable("Still valid")}));
	REQUIRE_FALSE(model->m_Changed.HasConnections());
}

TEST_CASE("Replacing a model during an edit notification preserves the replacement editor", "[data][editing][lifetime]")
{
	auto oldModel = std::make_shared<Yukiri::StandardItemModel>();
	auto newModel = std::make_shared<Yukiri::StandardItemModel>();
	auto oldId = oldModel->AppendRow({Editable("Old")}); REQUIRE(oldId);
	auto newId = newModel->AppendRow({Editable("New")}); REQUIRE(newId);
	auto view = std::make_unique<Yukiri::TableView>(); auto* table = view.get(); table->SetModel(oldModel);
	Yukiri::Detail::WidgetTree tree; tree.SetRoot(std::move(view)); tree.Layout({300, 180});
	auto connection = oldModel->m_Changed.Connect([&](const auto& change)
	{
		if (change.m_Kind != Yukiri::ItemModel::ChangeKind::Updated) return;
		table->SetModel(newModel);
		REQUIRE(table->BeginEdit(*newId));
	});
	REQUIRE(table->BeginEdit(*oldId));
	table->GetEditor()->SetText("Changed");
	tree.KeyDown({Yukiri::Key::Enter});
	REQUIRE(table->GetModel() == newModel);
	REQUIRE(table->GetEditor());
	REQUIRE(table->GetEditor()->GetText() == "New");
}

TEST_CASE("Table current cells support non-first-column editing and sorting retains checked values", "[data][table]")
{
	auto model = std::make_shared<Yukiri::StandardItemModel>(std::vector<std::string>{"Name", "Value"});
	auto check = Editable("C"); check.m_Checked = false;
	auto id = model->AppendRow({Editable("Name"), check}); REQUIRE(id);
	REQUIRE(model->AppendRow({Editable("Other"), Editable("B")}));
	auto view = std::make_unique<Yukiri::TableView>(); auto* table = view.get(); table->SetModel(model);
	Yukiri::Detail::WidgetTree tree; tree.SetRoot(std::move(view)); tree.Layout({240, 180}); tree.SetFocus(table);
	tree.KeyDown({Yukiri::Key::Home});
	tree.KeyDown({Yukiri::Key::Right}); REQUIRE(table->GetCurrentColumn() == 1);
	tree.KeyDown({Yukiri::Key::Space}); REQUIRE(model->GetCell(*id, 1).m_Checked == true);
	tree.KeyDown({Yukiri::Key::F2}); REQUIRE(table->GetEditingColumn() == 1);
	table->GetEditor()->SetText("A"); tree.KeyDown({Yukiri::Key::Enter});
	REQUIRE(model->GetCell(*id, 1).m_Text == "A");
	REQUIRE(table->GetCurrentColumn() == 1);
	REQUIRE(table->Sort(1, false));
	REQUIRE(model->SetText(*id, 1, "Z"));
	REQUIRE(model->GetChild(0, 0) == *id);
}
