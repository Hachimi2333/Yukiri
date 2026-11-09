#pragma once

#include <Yukiri/Yukiri.hpp>
#include <thread>

namespace YukiriDemo
{
[[nodiscard]] Yukiri::Result<Yukiri::Window*> OpenData(Yukiri::Application& application,
	Yukiri::WindowDecoration decoration, Yukiri::ThemeMode theme);
class DataWorkspace : public Yukiri::Grid
{
public:
	explicit DataWorkspace(Yukiri::Window& window);
	~DataWorkspace() override;
private:
	struct Lifetime { DataWorkspace* m_Owner; };
	void Add();
	void Remove();
	void Load();
	void Cancel();
	void Refresh();
	void AcceptBatch(std::uint64_t generation, std::vector<std::vector<Yukiri::ItemModel::Cell>> rows,
		std::size_t completed, std::size_t total);
	Yukiri::Window& m_Window;
	std::shared_ptr<Yukiri::StandardItemModel> m_Model;
	std::shared_ptr<Yukiri::SelectionModel> m_Selection;
	Yukiri::ListView* m_List = nullptr;
	Yukiri::TreeView* m_Tree = nullptr;
	Yukiri::TableView* m_Table = nullptr;
	Yukiri::Label* m_Status = nullptr;
	Yukiri::NumberBox* m_Count = nullptr;
	Yukiri::ProgressBar* m_Progress = nullptr;
	std::shared_ptr<Yukiri::Command> m_Add, m_Remove, m_Edit, m_Load, m_Cancel;
	std::shared_ptr<Lifetime> m_Lifetime = std::make_shared<Lifetime>(Lifetime{this});
	std::jthread m_Worker;
	std::uint64_t m_Generation = 0;
	std::size_t m_Loaded = 0;
	std::size_t m_Total = 0;
	bool m_Loading = false;
};
}
