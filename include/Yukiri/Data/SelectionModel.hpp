#pragma once

#include <Yukiri/Data/ItemModel.hpp>
#include <unordered_set>

namespace Yukiri
{
class SelectionModel : public std::enable_shared_from_this<SelectionModel>
{
public:
	enum class Mode { Single, Multiple };
	explicit SelectionModel(std::shared_ptr<ItemModel> model = {});
	void SetModel(std::shared_ptr<ItemModel> model);
	[[nodiscard]] const std::shared_ptr<ItemModel>& GetModel() const noexcept;
	void SetMode(Mode mode);
	[[nodiscard]] Mode GetMode() const noexcept;
	void Clear();
	void Select(ItemId item, bool add = false, bool toggle = false);
	void SetSelected(std::vector<ItemId> items, ItemId current = 0);
	void SetCurrent(ItemId current);
	[[nodiscard]] bool IsSelected(ItemId item) const;
	[[nodiscard]] const std::vector<ItemId>& GetSelected() const noexcept;
	[[nodiscard]] ItemId GetCurrent() const noexcept;
	Signal<> m_Changed;
	Signal<ItemId> m_CurrentChanged;
private:
	void AssertThread() const;
	void Prune(bool reset);
	std::thread::id m_Thread = std::this_thread::get_id();
	std::shared_ptr<ItemModel> m_Model;
	Connection m_Connection;
	std::vector<ItemId> m_Selected;
	std::unordered_set<ItemId> m_Index;
	ItemId m_Current = 0;
	Mode m_Mode = Mode::Single;
};
}
