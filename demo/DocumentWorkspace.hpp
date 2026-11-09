#pragma once

#include <Yukiri/Yukiri.hpp>
#include "DocumentFiles.hpp"
#include <functional>
#include <thread>

namespace YukiriDemo
{
[[nodiscard]] Yukiri::Result<Yukiri::Window*> OpenDocument(Yukiri::Application& application,
	Yukiri::WindowDecoration decoration, Yukiri::ThemeMode theme);

class DocumentWorkspace : public Yukiri::Grid
{
public:
	explicit DocumentWorkspace(Yukiri::Window& window);
	~DocumentWorkspace() override;
private:
	struct Lifetime { DocumentWorkspace* m_Owner; };
	using Action = std::function<void()>;
	std::shared_ptr<Yukiri::Command> MakeCommand(std::string text, Yukiri::Key key, bool shift, Action action);
	void GuardUnsaved(Action action);
	void ChooseOpen();
	void Read(std::filesystem::path path);
	void Save(bool saveAs, Action completed = {});
	void Write(std::filesystem::path path, Action completed);
	void Find(bool replace, bool all);
	void Refresh();
	void Error(std::string text);
	Yukiri::Window& m_Window;
	DocumentSession m_Document;
	Yukiri::TextArea* m_Editor = nullptr;
	Yukiri::TextBox* m_Find = nullptr;
	Yukiri::TextBox* m_Replacement = nullptr;
	Yukiri::CheckBox* m_MatchCase = nullptr;
	Yukiri::WrapPanel* m_FindPanel = nullptr;
	Yukiri::Label* m_Status = nullptr;
	Yukiri::ProgressBar* m_Progress = nullptr;
	std::shared_ptr<Yukiri::Command> m_New, m_Open, m_Save, m_SaveAs, m_Undo, m_Redo, m_FindCommand;
	std::shared_ptr<Lifetime> m_Lifetime = std::make_shared<Lifetime>(Lifetime{this});
	std::jthread m_Worker;
	bool m_Busy = false;
	bool m_Confirmation = false;
	bool m_AllowClose = false;
};
}
