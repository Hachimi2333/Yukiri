#pragma once

#include <Yukiri/Controls/ContextMenu.hpp>

namespace Yukiri
{
class MenuBar : public Widget
{
public:
	using Item = ContextMenu::Item;
	MenuBar();
	void SetItems(std::vector<Item> items);
	[[nodiscard]] const std::vector<Item>& GetItems() const noexcept;
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerMove(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
	void OnPaint(Canvas& canvas) override;
	Rect GetFocusBounds() const noexcept override;
private:
	friend class Detail::AccessibilityTree;
	std::uint64_t m_ItemsGeneration = 0;
	void Open(std::size_t index);
	void Move(int direction);
	void BindCommands();
	std::vector<Item> m_Items;
	std::vector<Rect> m_Headers;
	std::vector<Connection> m_Bindings;
	Window* m_BoundWindow = nullptr;
	std::size_t m_Current = 0;
	ContextMenu* m_Menu = nullptr;
	std::weak_ptr<int> m_MenuLifetime;
};
}
