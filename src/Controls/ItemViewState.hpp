#pragma once

#include <Yukiri/Controls/ListView.hpp>
#include <Yukiri/Controls/TextBox.hpp>
#include <unordered_map>
#include <unordered_set>

namespace Yukiri
{
struct ListView::Impl
{
	struct Row { ItemId m_Id; unsigned m_Depth; bool m_HasChildren; std::size_t m_Ordinal; };
	std::uint64_t m_ModelGeneration = 0;
	ViewKind m_Kind = ViewKind::List;
	std::shared_ptr<ItemModel> m_Model;
	std::shared_ptr<SelectionModel> m_Selection;
	Connection m_ModelConnection;
	std::vector<Connection> m_SelectionConnections;
	std::vector<Row> m_Rows;
	std::unordered_map<ItemId, std::size_t> m_Indices;
	std::unordered_set<ItemId> m_Expanded;
	std::vector<float> m_Columns;
	ItemId m_Root = 0;
	ItemId m_Anchor = 0;
	float m_RowHeight = 32;
	Rect m_Viewport;
	Point m_Offset;
	Point m_Target;
	Detail::Transition m_ScrollX;
	Detail::Transition m_ScrollY;
	int m_DragAxis = 0;
	bool m_DragSelecting = false;
	Modifiers m_DragModifiers;
	std::size_t m_ResizeColumn = 0;
	Point m_DragStart;
	Point m_DragOffset;
	float m_ResizeWidth = 0;
	TextBox* m_Editor = nullptr;
	ItemId m_EditingItem = 0;
	std::size_t m_EditingColumn = 0;
	std::size_t m_CurrentColumn = 0;
	bool m_Committing = false;
	bool m_Destroying = false;
	std::uint64_t m_EditVersion = 0;
};
}
