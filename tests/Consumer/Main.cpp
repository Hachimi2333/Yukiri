#include <Yukiri/Yukiri.hpp>
#include <iostream>

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
	std::cout << "Independent Yukiri consumer passed.\n";
}
