#include "AutomationProvider.hpp"
#include <Yukiri/Controls/ListView.hpp>
#include <array>
#include <limits>

namespace Yukiri::Detail
{
HRESULT AutomationProvider::GetItem(int row, int column, IRawElementProviderSimple** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Grid)) return UIA_E_NOTSUPPORTED;
		auto& view = static_cast<ListView&>(*m_Node.m_Owner);
		if (row < 0 || column < 0 || static_cast<std::size_t>(row) >= view.GetRowCount() || static_cast<std::size_t>(column) >= view.GetColumnCount()) return E_INVALIDARG;
		*result = m_Context->Get(m_Context->m_Tree->ForItem(view, view.GetItemAt(static_cast<std::size_t>(row)), static_cast<std::size_t>(column))).Detach();
		return S_OK;
	});
}
HRESULT AutomationProvider::get_RowCount(int* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Grid)) return UIA_E_NOTSUPPORTED; *result = static_cast<int>(std::min(static_cast<ListView&>(*m_Node.m_Owner).GetRowCount(), static_cast<std::size_t>(std::numeric_limits<int>::max()))); return S_OK; });
}
HRESULT AutomationProvider::get_ColumnCount(int* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Grid)) return UIA_E_NOTSUPPORTED; *result = static_cast<int>(std::min(static_cast<ListView&>(*m_Node.m_Owner).GetColumnCount(), static_cast<std::size_t>(std::numeric_limits<int>::max()))); return S_OK; });
}
HRESULT AutomationProvider::get_Row(int* result)
{
	if (!result) return E_POINTER;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::GridItem)) return UIA_E_NOTSUPPORTED;
		const auto row = static_cast<ListView&>(*m_Node.m_Owner).GetRowIndex(m_Node.m_Item);
		if (!row) return UIA_E_ELEMENTNOTAVAILABLE;
		*result = static_cast<int>(std::min(*row, static_cast<std::size_t>(std::numeric_limits<int>::max()))); return S_OK;
	});
}
HRESULT AutomationProvider::get_Column(int* result) { if (!result) return E_POINTER; return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::GridItem)) return UIA_E_NOTSUPPORTED; *result = static_cast<int>(m_Node.m_Index); return S_OK; }); }
HRESULT AutomationProvider::get_RowSpan(int* result) { if (!result) return E_POINTER; return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::GridItem)) return UIA_E_NOTSUPPORTED; *result = 1; return S_OK; }); }
HRESULT AutomationProvider::get_ColumnSpan(int* result) { return get_RowSpan(result); }
HRESULT AutomationProvider::get_ContainingGrid(IRawElementProviderSimple** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::GridItem)) return UIA_E_NOTSUPPORTED; *result = m_Context->Get(m_Context->m_Tree->ForWidget(*m_Node.m_Owner)).Detach(); return S_OK; });
}
HRESULT AutomationProvider::GetRowHeaders(SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Table)) return UIA_E_NOTSUPPORTED; return UnknownArray({}, result); });
}
HRESULT AutomationProvider::GetColumnHeaders(SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Table)) return UIA_E_NOTSUPPORTED;
		const auto header = m_Context->m_Tree->Child(m_Node, 0);
		std::vector<AccessibleNode> columns;
		if (header) for (std::size_t index = 0, count = m_Context->m_Tree->ChildCount(*header); index < count; ++index) columns.push_back(*m_Context->m_Tree->Child(*header, index));
		return m_Context->NodesArray(columns, result);
	});
}
HRESULT AutomationProvider::get_RowOrColumnMajor(RowOrColumnMajor* result) { if (!result) return E_POINTER; return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::Table)) return UIA_E_NOTSUPPORTED; *result = RowOrColumnMajor_RowMajor; return S_OK; }); }
HRESULT AutomationProvider::GetRowHeaderItems(SAFEARRAY** result) { if (!result) return E_POINTER; *result = nullptr; return Read([&]() -> HRESULT { if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::TableItem)) return UIA_E_NOTSUPPORTED; return UnknownArray({}, result); }); }
HRESULT AutomationProvider::GetColumnHeaderItems(SAFEARRAY** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::TableItem)) return UIA_E_NOTSUPPORTED;
		const auto header = m_Context->m_Tree->Child(m_Context->m_Tree->ForWidget(*m_Node.m_Owner), 0);
		const auto column = header ? m_Context->m_Tree->Child(*header, m_Node.m_Index) : std::nullopt;
		return column ? m_Context->NodesArray(std::array{*column}, result) : UnknownArray({}, result);
	});
}
HRESULT AutomationProvider::FindItemByProperty(IRawElementProviderSimple* after, PROPERTYID property, VARIANT value, IRawElementProviderSimple** result)
{
	if (!result) return E_POINTER;
	*result = nullptr;
	return Read([&]() -> HRESULT
	{
		if (!m_Context->m_Tree->HasPattern(m_Node, AccessiblePattern::ItemContainer)) return UIA_E_NOTSUPPORTED;
		if (property != 0 && property != UIA_NamePropertyId && property != UIA_AutomationIdPropertyId) return E_INVALIDARG;
		if (property != 0 && value.vt != VT_BSTR) return E_INVALIDARG;
		std::optional<AccessibleNode> previous;
		if (after)
		{
			previous = m_Context->Identity(after);
			if (!previous || previous->m_Owner != m_Node.m_Owner || !m_Context->m_Tree->IsAvailable(*previous) ||
				(previous->m_Kind != AccessibleKind::DataRow && previous->m_Kind != AccessibleKind::DataCell)) return E_INVALIDARG;
		}
		const auto text = property && value.bstrVal ? ToUtf8(std::wstring_view(value.bstrVal, SysStringLen(value.bstrVal))) : std::string{};
		auto& view = static_cast<ListView&>(*m_Node.m_Owner);
		const auto model = view.GetModel();
		if (!model) return S_OK;
		struct Frame { ItemId m_Parent; std::size_t m_Row; std::size_t m_Count; };
		std::vector<Frame> frames{{view.GetRootItem(), 0, model->GetRowCount(view.GetRootItem())}};
		bool active = !previous;
		const bool hierarchy = m_Context->m_Tree->Describe(m_Node).m_Info.m_Role == AccessibilityInfo::Role::Tree;
		while (!frames.empty())
		{
			auto& frame = frames.back();
			if (frame.m_Row >= frame.m_Count) { frames.pop_back(); continue; }
			const auto item = model->GetChild(frame.m_Parent, frame.m_Row++);
			const auto node = m_Context->m_Tree->ForItem(view, item);
			if (active)
			{
				bool matched = property == 0;
				if (property == UIA_NamePropertyId) matched = model->GetCell(item).m_Text == text;
				if (property == UIA_AutomationIdPropertyId) matched = m_Context->m_Tree->Describe(node).m_Info.m_AutomationId == text;
				if (matched) { *result = m_Context->Get(node).Detach(); return S_OK; }
			}
			if (previous && previous->m_Item == item) active = true;
			if (hierarchy) if (const auto count = model->GetRowCount(item)) frames.push_back({item, 0, count});
		}
		return S_OK;
	});
}
}
