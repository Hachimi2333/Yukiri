#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/Core/Dispatcher.hpp>

namespace Yukiri
{
class MenuBar;
class ContextMenu : public Widget
{
public:
	struct Item
	{
		std::string m_Text;
		std::shared_ptr<Command> m_Command;
		std::vector<Item> m_Children;
		bool m_Separator = false;
		[[nodiscard]] static Item Action(std::shared_ptr<Command> command);
		[[nodiscard]] static Item Submenu(std::string text, std::vector<Item> children);
		[[nodiscard]] static Item Divider();
	};
	explicit ContextMenu(std::vector<Item> items);
	[[nodiscard]] static Result<ContextMenu*> Show(Widget& owner, std::vector<Item> items, Point localPosition);
	static void Bind(Widget& owner, std::vector<Item> items);
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerUp(PointerEvent& event) override;
	void OnPointerMove(PointerEvent& event) override;
	void OnWheel(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	[[nodiscard]] const std::vector<Item>& GetItems() const noexcept;
	[[nodiscard]] std::optional<std::size_t> GetCurrentItem() const noexcept;
protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
	void OnPaint(Canvas& canvas) override;
	Rect GetFocusBounds() const noexcept override;
private:
	friend class Detail::AccessibilityTree;
	friend class MenuBar;
	[[nodiscard]] bool Selectable(std::size_t index) const;
	[[nodiscard]] float RowTop(std::size_t index) const;
	[[nodiscard]] float RowHeight(std::size_t index) const;
	[[nodiscard]] std::optional<std::size_t> HitRow(Point point) const;
	void Select(std::optional<std::size_t> index, bool keyboard);
	void Move(int direction);
	void InvokeCurrent();
	void OpenSubmenu();
	void Dismiss();
	std::vector<Item> m_Items;
	std::optional<std::size_t> m_Current;
	std::optional<std::size_t> m_Pressed;
	ContextMenu* m_Root = this;
	ContextMenu* m_Child = nullptr;
	std::weak_ptr<int> m_ChildLifetime;
	std::weak_ptr<int> m_RootLifetime;
	std::function<void(int)> m_SwitchMenu;
	Timer m_HoverTimer;
	float m_Scroll = 0;
	float m_ContentHeight = 0;
};
}
