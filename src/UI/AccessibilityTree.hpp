#pragma once

#include <Yukiri/UI/AccessibilityInfo.hpp>
#include <Yukiri/Core/Error.hpp>
#include <Yukiri/Core/Geometry.hpp>
#include <Yukiri/Data/ItemModel.hpp>
#include <functional>

namespace Yukiri
{
class Widget;
class ListView;
class TextBox;
namespace Detail
{
class WidgetTree;
enum class AccessibleKind { Window, TitleBar, CaptionButton, Widget, RadioGroup, TabItem, ComboItem, MenuItem, MenuHeader, DataRow, DataCell, HeaderRow, HeaderCell, SplitGrip };
enum class AccessiblePattern : std::uint64_t
{
	Invoke = 1, Toggle = 2, Selection = 4, SelectionItem = 8, ExpandCollapse = 16,
	Value = 32, RangeValue = 64, Scroll = 128, ScrollItem = 256, Grid = 512,
	GridItem = 1024, Table = 2048, TableItem = 4096, Text = 8192, ItemContainer = 16384,
	VirtualizedItem = 32768, Window = 65536
};
struct AccessibleNode
{
	AccessibleKind m_Kind = AccessibleKind::Window;
	Widget* m_Owner = nullptr;
	std::weak_ptr<int> m_Lifetime;
	std::uint64_t m_WidgetId = 0;
	ItemId m_Item = 0;
	std::size_t m_Index = 0;
	std::uint64_t m_Generation = 0;
	bool operator==(const AccessibleNode& other) const noexcept
	{
		return m_Kind == other.m_Kind && m_WidgetId == other.m_WidgetId && m_Item == other.m_Item && m_Index == other.m_Index && m_Generation == other.m_Generation;
	}
};
struct AccessibleState
{
	AccessibilityInfo m_Info;
	Rect m_Bounds;
	std::uint64_t m_Patterns = 0;
	bool m_Enabled = true;
	bool m_Visible = true;
	bool m_Focusable = false;
	bool m_Focused = false;
	bool m_Selected = false;
	bool m_Checked = false;
	bool m_Expanded = false;
	bool m_Leaf = true;
	bool m_Password = false;
	bool m_ReadOnly = false;
	bool m_Modal = false;
	std::string m_Value;
	std::string m_Accelerator;
	std::string m_AccessKey;
	std::string m_ItemStatus;
};
struct AccessibleRange { double m_Value; double m_Minimum; double m_Maximum; double m_SmallChange; double m_LargeChange; bool m_ReadOnly; };
struct AccessibleScroll { Point m_Offset; Size m_Extent; Size m_Viewport; };
class AccessibilityTree
{
public:
	explicit AccessibilityTree(WidgetTree& tree);
	[[nodiscard]] AccessibleNode Root() const;
	[[nodiscard]] AccessibleNode ForWidget(Widget& widget) const;
	[[nodiscard]] AccessibleNode ForItem(ListView& view, ItemId item, std::optional<std::size_t> column = {}) const;
	[[nodiscard]] bool IsAvailable(const AccessibleNode& node) const;
	[[nodiscard]] AccessibleState Describe(const AccessibleNode& node) const;
	[[nodiscard]] std::size_t ChildCount(const AccessibleNode& node) const;
	[[nodiscard]] std::optional<AccessibleNode> Child(const AccessibleNode& node, std::size_t index) const;
	[[nodiscard]] std::optional<AccessibleNode> Parent(const AccessibleNode& node) const;
	[[nodiscard]] std::optional<AccessibleNode> Sibling(const AccessibleNode& node, bool next) const;
	[[nodiscard]] std::optional<AccessibleNode> Focus() const;
	[[nodiscard]] std::optional<AccessibleNode> AtPoint(Point point) const;
	[[nodiscard]] std::vector<AccessibleNode> Selection(const AccessibleNode& node) const;
	[[nodiscard]] bool CanSelectMultiple(const AccessibleNode& node) const;
	[[nodiscard]] std::optional<AccessibleRange> Range(const AccessibleNode& node) const;
	[[nodiscard]] std::optional<AccessibleScroll> ScrollInfo(const AccessibleNode& node) const;
	[[nodiscard]] TextBox* TextControl(const AccessibleNode& node) const;
	[[nodiscard]] std::vector<std::size_t> TextBoundaries(const AccessibleNode& node, int unit) const;
	[[nodiscard]] std::pair<std::size_t, std::size_t> VisibleText(const AccessibleNode& node) const;
	[[nodiscard]] std::vector<Rect> TextRectangles(const AccessibleNode& node, std::size_t start, std::size_t end) const;
	[[nodiscard]] bool HasPattern(const AccessibleNode& node, AccessiblePattern pattern) const;
	[[nodiscard]] std::string Value(const AccessibleNode& node) const;
	[[nodiscard]] std::uint64_t TextVersion(const AccessibleNode& node) const;
	[[nodiscard]] std::uint64_t SelectionVersion(const AccessibleNode& node) const;
	Result<void> Invoke(const AccessibleNode& node);
	Result<void> Toggle(const AccessibleNode& node);
	Result<void> SetFocus(const AccessibleNode& node);
	Result<void> Select(const AccessibleNode& node, bool add = false, bool remove = false);
	Result<void> SetExpanded(const AccessibleNode& node, bool expanded);
	Result<void> SetValue(const AccessibleNode& node, std::string value);
	Result<void> SetRangeValue(const AccessibleNode& node, double value);
	Result<void> SetScroll(const AccessibleNode& node, Point offset);
	Result<void> Realize(const AccessibleNode& node);
	Result<void> ScrollText(const AccessibleNode& node, std::size_t start, std::size_t end, bool top);
	Result<void> WindowAction(const AccessibleNode& node, int action);
	void Dispatch(std::function<void()> action) const;
	void EnsureLayout() const;
	static void StructureChanged(Widget& widget);
	[[nodiscard]] std::uint64_t StructureVersion() const;
	[[nodiscard]] bool HasCustomTitleBar() const;
private:
	AccessibleNode Part(AccessibleKind kind, Widget& owner, std::size_t index = 0) const;
	std::vector<AccessibleNode> WidgetChildren(Widget& widget) const;
	Rect Clip(Widget& widget, Rect bounds) const;
	bool Active(Widget& widget) const;
	Widget* PopupOwner(const Widget& widget) const;
	bool Enabled(Widget& widget) const;
	std::size_t Ordinal(const AccessibleNode& node) const;
	WidgetTree* m_Tree;
	std::weak_ptr<int> m_Lifetime;
};
}
}
