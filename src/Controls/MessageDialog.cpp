#include <Yukiri/Controls/MessageDialog.hpp>
#include <Yukiri/Controls/Label.hpp>

namespace Yukiri
{
Result<Dialog*> MessageDialog::Show(Window& window, std::string title, std::string text, Buttons buttons,
	std::function<void(Dialog::Result)> completed)
{
	auto dialog = std::make_unique<Dialog>(std::move(title));
	auto label = std::make_unique<Label>(std::move(text));
	label->SetWrap(true);
	dialog->SetContent(std::move(label));
	using D = Dialog::Result;
	switch (buttons)
	{
	case Buttons::Ok: dialog->SetButtons({{"OK", D::Ok}}, D::Ok, D::Ok); break;
	case Buttons::OkCancel: dialog->SetButtons({{"OK", D::Ok}, {"Cancel", D::Cancel}}); break;
	case Buttons::YesNo: dialog->SetButtons({{"Yes", D::Yes}, {"No", D::No}}, D::Yes, std::nullopt); break;
	case Buttons::YesNoCancel: dialog->SetButtons({{"Yes", D::Yes}, {"No", D::No}, {"Cancel", D::Cancel}}, D::Yes); break;
	case Buttons::SaveDiscardCancel: dialog->SetButtons({{"Save", D::Save}, {"Discard", D::Discard}, {"Cancel", D::Cancel}}, D::Save); break;
	}
	if (completed) dialog->Keep(dialog->m_Completed.Connect(std::move(completed)));
	return Dialog::Show(window, std::move(dialog));
}
}
