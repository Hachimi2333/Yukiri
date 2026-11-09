#include <Yukiri/Yukiri.hpp>
#include <iostream>
#include <array>

int main()
{
	volatile auto factory = &Yukiri::Application::Create;
	if (!factory || Yukiri::GetVersion() != "0.1.0") return 1;
	Yukiri::StackPanel panel;
	auto& button = panel.AddChild<Yukiri::Button>("Consumer");
	bool activated = false;
	auto connection = button.m_Clicked.Connect([&] { activated = true; });
	panel.Measure({400, 200});
	panel.Arrange({0, 0, 400, 200});
	Yukiri::KeyEvent event{Yukiri::Key::Enter};
	button.OnKeyDown(event);
	if (!activated || panel.GetChildren().size() != 1) return 2;
	volatile auto picker = &Yukiri::FileDialog::Open;
	if (!picker) return 3;
	auto image = Yukiri::Image::FromRgba(1, 1, std::array<std::uint8_t, 4>{255, 255, 255, 255});
	if (!image) return 4;
	Yukiri::Dialog dialog("Consumer dialog");
	dialog.SetContent(std::make_unique<Yukiri::TextArea>("Line one\nLine two"));
	Yukiri::NumberBox number(1);
	auto model = std::make_shared<Yukiri::StandardItemModel>();
	Yukiri::ItemModel::Cell cell; cell.m_Text = "A row"; cell.m_Editable = true;
	const auto row = model->AppendRow({cell});
	if (!row) return 5;
	Yukiri::TableView table; table.SetModel(model);
	table.Measure({400, 200}); table.Arrange({0, 0, 400, 200});
	if (!table.Sort(0) || !table.BeginEdit(*row)) return 6;
	table.CancelEdit();
	std::cout << "Independent Yukiri consumer passed.\n";
}
