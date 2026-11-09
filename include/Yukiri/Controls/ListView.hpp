#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/Data/SelectionModel.hpp>
#include <span>

namespace Yukiri
{
class TextBox;
class ListView : public Widget
{
public:
	ListView();
	~ListView() override;
	void SetModel(std::shared_ptr<ItemModel> model);
	[[nodiscard]] const std::shared_ptr<ItemModel>& GetModel() const noexcept;
	[[nodiscard]] const std::shared_ptr<SelectionModel>& GetSelectionModel() const noexcept;
	[[nodiscard]] Result<void> SetSelectionModel(std::shared_ptr<SelectionModel> selection);
	void SetSelectionMode(SelectionModel::Mode mode);
	[[nodiscard]] Result<void> SetRootItem(ItemId item);
	[[nodiscard]] ItemId GetRootItem() const noexcept;
	void SetRowHeight(float height);
	[[nodiscard]] float GetRowHeight() const noexcept;
	[[nodiscard]] std::size_t GetRowCount() const noexcept;
	[[nodiscard]] std::size_t GetColumnCount() const noexcept;
	[[nodiscard]] std::size_t GetCurrentColumn() const noexcept;
	[[nodiscard]] Result<void> SetCurrentColumn(std::size_t column);
	[[nodiscard]] ItemId GetItemAt(std::size_t row) const noexcept;
	[[nodiscard]] std::optional<std::size_t> GetRowIndex(ItemId item) const;
	[[nodiscard]] std::pair<std::size_t, std::size_t> GetVisibleRange() const;
	[[nodiscard]] Rect GetItemBounds(ItemId item, std::size_t column = 0) const;
	void ScrollIntoView(ItemId item);
	void SetOffset(Point offset);
	[[nodiscard]] Point GetOffset() const noexcept;
	void SetExpanded(ItemId item, bool expanded);
	[[nodiscard]] bool IsExpanded(ItemId item) const;
	[[nodiscard]] Result<void> BeginEdit(ItemId item, std::size_t column = 0);
	[[nodiscard]] Result<void> CommitEdit();
	void CancelEdit();
	[[nodiscard]] TextBox* GetEditor() const noexcept;
	[[nodiscard]] ItemId GetEditingItem() const noexcept;
	[[nodiscard]] std::size_t GetEditingColumn() const noexcept;
	void OnPointerDown(PointerEvent& event) override;
	void OnPointerUp(PointerEvent& event) override;
	void OnPointerMove(PointerEvent& event) override;
	void OnPointerCaptureLost() override;
	void OnWheel(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	Signal<ItemId> m_ItemActivated;
	Signal<ItemId, bool> m_ExpandedChanged;
protected:
	enum class ViewKind { List, Tree, Table };
	explicit ListView(ViewKind kind);
	[[nodiscard]] Result<void> SetColumnWidthImpl(std::size_t column, float width);
	[[nodiscard]] float GetColumnWidthImpl(std::size_t column) const;
	[[nodiscard]] Result<void> SortImpl(std::size_t column, bool ascending);
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
	void OnPaint(Canvas& canvas) override;
	bool OnAnimate(std::chrono::nanoseconds delta) override;
	Rect GetChildrenClip() const noexcept override;
	Rect GetFocusBounds() const noexcept override;
private:
	friend class Detail::AccessibilityTree;
	struct Impl;
	class Editor;
	std::unique_ptr<Impl> m_Impl;
	void RebuildRows();
	void BindSelection();
	void ModelChanged(const ItemModel::Change& change);
	void LayoutViewport();
	void ArrangeEditor();
	void ScrollTo(Point offset);
	[[nodiscard]] Point ClampOffset(Point offset) const;
	[[nodiscard]] float HeaderHeight() const;
	[[nodiscard]] float ContentWidth() const;
	[[nodiscard]] float ColumnLeft(std::size_t column) const;
	[[nodiscard]] Rect CellBounds(std::size_t row, std::size_t column) const;
	[[nodiscard]] Rect VerticalThumb() const;
	[[nodiscard]] Rect HorizontalThumb() const;
	[[nodiscard]] std::optional<std::size_t> RowAt(Point local) const;
	[[nodiscard]] std::optional<std::size_t> ColumnAt(float x) const;
	void SelectRow(std::size_t row, Modifiers modifiers);
	void MoveCurrent(int direction, bool edge, Modifiers modifiers);
};
}
