#include "Platform/AutomationProvider.hpp"
#include "Platform/UiApartment.hpp"
#include "UI/WidgetTree.hpp"
#include "Text/Unicode.hpp"
#include <Yukiri/Controls/Button.hpp>
#include <Yukiri/Controls/TextArea.hpp>
#include <Yukiri/Controls/TableView.hpp>
#include <Yukiri/Controls/Slider.hpp>
#include <Yukiri/Controls/ToggleSwitch.hpp>
#include <Yukiri/Controls/Dialog.hpp>
#include <Yukiri/Layout/StackPanel.hpp>
#include <catch2/catch_test_macros.hpp>
#include <future>

namespace
{
using Yukiri::Detail::ComPtr;
template<typename T> ComPtr<T> Query(IUnknown* provider)
{
	ComPtr<T> result;
	REQUIRE(provider->QueryInterface(__uuidof(T), reinterpret_cast<void**>(result.Put())) == S_OK);
	return result;
}
template<typename T> ComPtr<T> Pattern(IRawElementProviderSimple* provider, PATTERNID id)
{
	ComPtr<IUnknown> pattern;
	REQUIRE(provider->GetPatternProvider(id, pattern.Put()) == S_OK);
	REQUIRE(pattern.Get());
	return Query<T>(pattern.Get());
}
std::string Text(ITextRangeProvider* range, int maximum = -1)
{
	BSTR value = nullptr;
	REQUIRE(range->GetText(maximum, &value) == S_OK);
	const auto result = value ? Yukiri::Detail::ToUtf8(std::wstring_view(value, SysStringLen(value))) : std::string{};
	SysFreeString(value); return result;
}
void Metrics(Yukiri::Detail::WidgetTree& tree)
{
	tree.m_MeasureText = [](auto text, auto, const auto&) { return Yukiri::Size{static_cast<float>(Yukiri::Detail::GraphemeBoundaries(text).size() - 1) * 8, 20}; };
}
}
TEST_CASE("UI Automation providers invoke safely and reject stale and wrong-thread access without windows", "[automation]")
{
	auto apartment = Yukiri::Detail::UiApartment::Create(); REQUIRE(apartment);
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>(); auto* panel = root.get();
	auto& button = root->AddChild<Yukiri::Button>("Remove me");
	button.Keep(button.m_Clicked.Connect([&] { panel->RemoveChild(button); }));
	tree.SetRoot(std::move(root)); tree.Layout({300, 100});
	auto context = std::make_shared<Yukiri::Detail::AutomationContext>(tree);
	auto provider = context->Get(context->m_Tree->ForWidget(button));
	const auto status = std::async(std::launch::async, [&] { VARIANT value{}; return provider->GetPropertyValue(UIA_NamePropertyId, &value); }).get();
	REQUIRE(status == RPC_E_WRONG_THREAD);
	auto invoke = Pattern<IInvokeProvider>(provider.Get(), UIA_InvokePatternId);
	REQUIRE(invoke->Invoke() == S_OK);
	VARIANT name{}; REQUIRE(provider->GetPropertyValue(UIA_NamePropertyId, &name) == UIA_E_ELEMENTNOTAVAILABLE);
	REQUIRE(panel->GetChildren().empty());
}
TEST_CASE("UI Automation range, toggle, and event-subscription mappings use logical values", "[automation][patterns]")
{
	auto apartment = Yukiri::Detail::UiApartment::Create(); REQUIRE(apartment);
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::StackPanel>();
	auto& slider = root->AddChild<Yukiri::Slider>(); REQUIRE(slider.SetRange(0, 10)); REQUIRE(slider.SetStep(0.5));
	auto& toggle = root->AddChild<Yukiri::ToggleSwitch>("Preview");
	tree.SetRoot(std::move(root)); tree.Layout({300, 140});
	auto context = std::make_shared<Yukiri::Detail::AutomationContext>(tree);
	auto sliderProvider = context->Get(context->m_Tree->ForWidget(slider));
	auto range = Pattern<IRangeValueProvider>(sliderProvider.Get(), UIA_RangeValuePatternId);
	REQUIRE(range->SetValue(3.5) == S_OK); REQUIRE(slider.GetValue() == 3.5f);
	REQUIRE(range->SetValue(11) == E_INVALIDARG);
	auto toggleProvider = context->Get(context->m_Tree->ForWidget(toggle));
	auto action = Pattern<IToggleProvider>(toggleProvider.Get(), UIA_TogglePatternId);
	REQUIRE(action->Toggle() == S_OK); REQUIRE(toggle.IsChecked());
	const auto observers = Query<IRawElementProviderAdviseEvents>(sliderProvider.Get());
	REQUIRE(observers->AdviseEventAdded(UIA_AutomationPropertyChangedEventId, nullptr) == S_OK);
	REQUIRE(context->IsPropertyObserved(UIA_ValueValuePropertyId));
	REQUIRE(observers->AdviseEventRemoved(UIA_AutomationPropertyChangedEventId, nullptr) == S_OK);
	REQUIRE_FALSE(context->IsPropertyObserved(UIA_ValueValuePropertyId));
}
TEST_CASE("UI Automation text ranges preserve Unicode, selection, peers, and edits", "[automation][text]")
{
	auto apartment = Yukiri::Detail::UiApartment::Create(); REQUIRE(apartment);
	Yukiri::Detail::WidgetTree tree; Metrics(tree);
	auto editor = std::make_unique<Yukiri::TextArea>("Alpha 中e\xCC\x81\nBeta"); auto* text = editor.get();
	tree.SetRoot(std::move(editor)); tree.Layout({240, 140}); tree.SetFocus(text);
	auto context = std::make_shared<Yukiri::Detail::AutomationContext>(tree);
	auto provider = context->Get(context->m_Tree->ForWidget(*text));
	auto pattern = Pattern<ITextProvider2>(provider.Get(), UIA_TextPattern2Id);
	ComPtr<ITextRangeProvider> document; REQUIRE(pattern->get_DocumentRange(document.Put()) == S_OK);
	REQUIRE(Text(document.Get()) == text->GetText());
	BSTR needle = SysAllocString(L"beta");
	ComPtr<ITextRangeProvider> found; REQUIRE(document->FindText(needle, FALSE, TRUE, found.Put()) == S_OK); SysFreeString(needle);
	REQUIRE(found.Get()); REQUIRE(Text(found.Get()) == "Beta");
	REQUIRE(found->Select() == S_OK);
	REQUIRE(text->GetText().substr(text->GetSelection().first) == "Beta");
	ComPtr<ITextRangeProvider> clone; REQUIRE(found->Clone(clone.Put()) == S_OK);
	BOOL same = FALSE; REQUIRE(found->Compare(clone.Get(), &same) == S_OK); REQUIRE(same);
	text->SetSelection(0, 0); tree.TextInput("X");
	REQUIRE(Text(found.Get()) == "Beta");
	int moved = 0; REQUIRE(found->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, -1, &moved) == S_OK);
	REQUIRE(moved == -1); REQUIRE(Text(found.Get()) == "Bet");
	REQUIRE(found->ExpandToEnclosingUnit(TextUnit_Line) == S_OK); REQUIRE(Text(found.Get()) == "Beta");
	SAFEARRAY* bounds = nullptr; REQUIRE(found->GetBoundingRectangles(&bounds) == S_OK); REQUIRE(bounds); SafeArrayDestroy(bounds);
	ComPtr<IUnknown> textIdentity = Query<IUnknown>(pattern.Get()), providerIdentity = Query<IUnknown>(provider.Get());
	REQUIRE(textIdentity.Get() == providerIdentity.Get());
}
TEST_CASE("UI Automation password interfaces never expose plaintext even through retained ranges", "[automation][password]")
{
	auto apartment = Yukiri::Detail::UiApartment::Create(); REQUIRE(apartment);
	Yukiri::Detail::WidgetTree tree; Metrics(tree);
	auto box = std::make_unique<Yukiri::TextBox>("secret"); auto* text = box.get();
	tree.SetRoot(std::move(box)); tree.Layout({240, 40});
	auto context = std::make_shared<Yukiri::Detail::AutomationContext>(tree);
	auto provider = context->Get(context->m_Tree->ForWidget(*text));
	auto pattern = Pattern<ITextProvider>(provider.Get(), UIA_TextPatternId);
	ComPtr<ITextRangeProvider> range; REQUIRE(pattern->get_DocumentRange(range.Put()) == S_OK);
	text->SetPassword(true);
	BSTR value = nullptr; REQUIRE(range->GetText(-1, &value) == E_ACCESSDENIED); REQUIRE(value == nullptr);
	auto valuePattern = Pattern<IValueProvider>(provider.Get(), UIA_ValuePatternId);
	REQUIRE(valuePattern->get_Value(&value) == E_ACCESSDENIED); REQUIRE(value == nullptr);
	ComPtr<IUnknown> unavailable; REQUIRE(provider->GetPatternProvider(UIA_TextPatternId, unavailable.Put()) == S_OK); REQUIRE_FALSE(unavailable.Get());
	SAFEARRAY* selection = nullptr;
	const auto wrongPattern = Query<ISelectionProvider>(provider.Get());
	REQUIRE(wrongPattern->GetSelection(&selection) == UIA_E_NOTSUPPORTED); REQUIRE(selection == nullptr);
	REQUIRE(valuePattern->SetValue(L"changed") == S_OK); REQUIRE(text->GetText() == "changed");
}
TEST_CASE("UI Automation character ranges honor graphemes and never truncate surrogate pairs", "[automation][unicode]")
{
	auto apartment = Yukiri::Detail::UiApartment::Create(); REQUIRE(apartment);
	Yukiri::Detail::WidgetTree tree; Metrics(tree);
	tree.SetRoot(std::make_unique<Yukiri::TextBox>("A👩‍💻B")); tree.Layout({240, 40});
	auto context = std::make_shared<Yukiri::Detail::AutomationContext>(tree);
	auto provider = context->Get(context->m_Tree->ForWidget(*tree.m_Root));
	auto pattern = Pattern<ITextProvider>(provider.Get(), UIA_TextPatternId);
	ComPtr<ITextRangeProvider> range; REQUIRE(pattern->get_DocumentRange(range.Put()) == S_OK);
	int moved = 0;
	REQUIRE(range->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Document, -1, &moved) == S_OK);
	REQUIRE(range->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 2, &moved) == S_OK);
	REQUIRE(moved == 2); REQUIRE(Text(range.Get()) == "A👩‍💻");
	REQUIRE(Text(range.Get(), 2) == "A");
}

TEST_CASE("UI Automation virtual table cells are created on demand and realized without widgets", "[automation][virtualization]")
{
	class Model : public Yukiri::ItemModel
	{
	public:
		mutable unsigned m_DataCalls = 0;
		std::size_t GetRowCount(Yukiri::ItemId parent) const override { return parent ? 0 : 100000; }
		std::size_t GetColumnCount() const override { return 5; }
		Yukiri::ItemId GetChild(Yukiri::ItemId parent, std::size_t row) const override { return !parent && row < 100000 ? row + 1 : 0; }
		Yukiri::ItemId GetParent(Yukiri::ItemId) const override { return 0; }
		bool Contains(Yukiri::ItemId item) const override { return item >= 1 && item <= 100000; }
		Cell GetCell(Yukiri::ItemId item, std::size_t) const override { ++m_DataCalls; Cell cell; cell.m_Text = "Row " + std::to_string(item); return cell; }
	};
	auto apartment = Yukiri::Detail::UiApartment::Create(); REQUIRE(apartment);
	auto model = std::make_shared<Model>();
	auto view = std::make_unique<Yukiri::TableView>(); auto* table = view.get(); view->SetModel(model);
	Yukiri::Detail::WidgetTree tree; tree.SetRoot(std::move(view)); tree.Layout({240, 180});
	auto context = std::make_shared<Yukiri::Detail::AutomationContext>(tree);
	auto provider = context->Get(context->m_Tree->ForWidget(*table));
	auto grid = Pattern<IGridProvider>(provider.Get(), UIA_GridPatternId);
	const auto baseline = context->m_Providers.size();
	{
		ComPtr<IRawElementProviderSimple> cell; REQUIRE(grid->GetItem(99999, 4, cell.Put()) == S_OK);
		REQUIRE(context->m_Providers.size() == baseline + 1);
		auto item = Pattern<IGridItemProvider>(cell.Get(), UIA_GridItemPatternId);
		int row = 0, column = 0; REQUIRE(item->get_Row(&row) == S_OK); REQUIRE(item->get_Column(&column) == S_OK);
		REQUIRE(row == 99999); REQUIRE(column == 4);
		auto virtualized = Pattern<IVirtualizedItemProvider>(cell.Get(), UIA_VirtualizedItemPatternId);
		REQUIRE(virtualized->Realize() == S_OK);
		UiaRect bounds{}; auto fragment = Query<IRawElementProviderFragment>(cell.Get());
		REQUIRE(fragment->get_BoundingRectangle(&bounds) == S_OK); REQUIRE(bounds.width > 0); REQUIRE(bounds.height > 0);
		REQUIRE(table->GetCurrentColumn() == 0);
		REQUIRE(table->GetChildren().empty());
	}
	REQUIRE(context->m_Providers.size() == baseline);
	REQUIRE(model->m_DataCalls < 30);
}

TEST_CASE("UI Automation modal ranges block owner actions and close through normal dialog results", "[automation][modal]")
{
	auto apartment = Yukiri::Detail::UiApartment::Create(); REQUIRE(apartment);
	Yukiri::Detail::WidgetTree tree; tree.m_Theme.m_Motion.m_Enabled = false;
	auto button = std::make_unique<Yukiri::Button>("Outside"); auto* outside = button.get();
	tree.SetRoot(std::move(button)); tree.Layout({600, 400}); tree.SetFocus(outside);
	auto context = std::make_shared<Yukiri::Detail::AutomationContext>(tree);
	auto outsideProvider = context->Get(context->m_Tree->ForWidget(*outside));
	auto dialog = std::make_unique<Yukiri::Dialog>("Confirmation"); auto* control = dialog.get();
	int completed = 0;
	auto connection = dialog->m_Completed.Connect([&](auto result) { REQUIRE(result == Yukiri::Dialog::Result::Cancel); ++completed; });
	REQUIRE(tree.OpenPopup(nullptr, std::move(dialog), {}, true)); tree.Layout({600, 400});
	auto modalProvider = context->Get(context->m_Tree->ForWidget(*control));
	auto window = Pattern<IWindowProvider>(modalProvider.Get(), UIA_WindowPatternId);
	BOOL modal = FALSE; REQUIRE(window->get_IsModal(&modal) == S_OK); REQUIRE(modal);
	auto invoke = Pattern<IInvokeProvider>(outsideProvider.Get(), UIA_InvokePatternId);
	REQUIRE(invoke->Invoke() == UIA_E_ELEMENTNOTENABLED);
	REQUIRE(window->Close() == S_OK); REQUIRE(completed == 1); REQUIRE(outside->HasFocus());
	VARIANT name{}; REQUIRE(modalProvider->GetPropertyValue(UIA_NamePropertyId, &name) == UIA_E_ELEMENTNOTAVAILABLE);
}

TEST_CASE("UI Automation text and element references become unavailable after their tree is destroyed", "[automation][lifetime]")
{
	auto apartment = Yukiri::Detail::UiApartment::Create(); REQUIRE(apartment);
	auto tree = std::make_unique<Yukiri::Detail::WidgetTree>();
	tree->SetRoot(std::make_unique<Yukiri::TextBox>("Transient")); tree->Layout({240, 40});
	auto context = std::make_shared<Yukiri::Detail::AutomationContext>(*tree);
	auto provider = context->Get(context->m_Tree->ForWidget(*tree->m_Root));
	auto text = Pattern<ITextProvider>(provider.Get(), UIA_TextPatternId);
	ComPtr<ITextRangeProvider> range; REQUIRE(text->get_DocumentRange(range.Put()) == S_OK);
	tree.reset();
	BSTR value = nullptr; REQUIRE(range->GetText(-1, &value) == UIA_E_ELEMENTNOTAVAILABLE); REQUIRE(value == nullptr);
	VARIANT name{}; REQUIRE(provider->GetPropertyValue(UIA_NamePropertyId, &name) == UIA_E_ELEMENTNOTAVAILABLE);
}
