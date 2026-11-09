#include "ItemViewState.hpp"

namespace Yukiri
{
class ListView::Editor : public TextBox
{
public:
	Editor(ListView& owner, std::string text) : TextBox(std::move(text)), m_Owner(owner), m_Lifetime(ObserveLifetime(owner)) {}
	void OnFocusChanged(bool focused) override
	{
		TextBox::OnFocusChanged(focused);
		if (!focused && !m_Lifetime.expired() && !m_Owner.m_Impl->m_Destroying && !m_Owner.m_Impl->m_Committing && m_Owner.GetEditor() == this)
		{ const auto committed = m_Owner.CommitEdit(); (void)committed; }
	}
private:
	ListView& m_Owner;
	std::weak_ptr<int> m_Lifetime;
};
TextBox* ListView::GetEditor() const noexcept { return m_Impl->m_Editor; }
ItemId ListView::GetEditingItem() const noexcept { return m_Impl->m_EditingItem; }
std::size_t ListView::GetEditingColumn() const noexcept { return m_Impl->m_EditingColumn; }
Result<void> ListView::BeginEdit(ItemId item, std::size_t column)
{
	if (!m_Impl->m_Model || !m_Impl->m_Model->Contains(item) || column >= GetColumnCount()) return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid edit cell."});
	const auto model = m_Impl->m_Model;
	const auto cell = model->GetCell(item, column);
	if (!cell.m_Editable || !cell.m_Enabled) return std::unexpected(Error{ErrorCode::InvalidArgument, "The cell is not editable."});
	Result<void> result;
	Dispatch([&]
	{
		if (m_Impl->m_Editor) { result = CommitEdit(); if (!result) return; }
		if (m_Impl->m_Model != model || !model->Contains(item)) { result = std::unexpected(Error{ErrorCode::InvalidArgument, "The model changed while starting the editor."}); return; }
		m_Impl->m_CurrentColumn = column;
		ScrollIntoView(item);
		if (!GetRowIndex(item)) { result = std::unexpected(Error{ErrorCode::InvalidArgument, "The cell is outside the view root."}); return; }
		m_Impl->m_EditingItem = item; m_Impl->m_EditingColumn = column;
		m_Impl->m_Editor = &AddChild<Editor>(*this, cell.m_Text);
		const auto version = ++m_Impl->m_EditVersion;
		m_Impl->m_Editor->Keep(m_Impl->m_Editor->m_Submitted.Connect([this] { const auto committed = CommitEdit(); (void)committed; }));
		m_Impl->m_Editor->Measure({GetColumnWidthImpl(column), GetRowHeight()});
		ArrangeEditor();
		auto* editor = m_Impl->m_Editor;
		editor->RequestFocus();
		if (m_Impl->m_EditVersion == version && m_Impl->m_Editor == editor) editor->SelectAll();
	});
	return result;
}
void ListView::ArrangeEditor()
{
	if (!m_Impl->m_Editor) return;
	const auto row = GetRowIndex(m_Impl->m_EditingItem);
	if (!row) return;
	auto rect = CellBounds(*row, m_Impl->m_EditingColumn);
	const auto bounds = GetBounds();
	rect.m_X += bounds.m_X + 1; rect.m_Y += bounds.m_Y + 1;
	rect.m_Width = std::max(0.0f, rect.m_Width - 2); rect.m_Height = std::max(0.0f, rect.m_Height - 2);
	m_Impl->m_Editor->Arrange(rect);
}
Result<void> ListView::CommitEdit()
{
	if (!m_Impl->m_Editor || m_Impl->m_Committing) return {};
	Result<void> result;
	Dispatch([&]
	{
		m_Impl->m_Committing = true;
		auto* editor = m_Impl->m_Editor;
		const auto version = m_Impl->m_EditVersion;
		const auto model = m_Impl->m_Model;
		const auto item = m_Impl->m_EditingItem;
		result = model->SetText(item, m_Impl->m_EditingColumn, editor->GetText());
		m_Impl->m_Committing = false;
		if (m_Impl->m_EditVersion != version || m_Impl->m_Editor != editor) return;
		if (!result)
		{
			WidgetStyle style = editor->GetStyle(); style.m_Border = Color::FromHex(0xD94D4D);
			editor->SetStyle(style); editor->SetToolTip(result.error().m_Message);
			return;
		}
		CancelEdit();
	});
	return result;
}
void ListView::CancelEdit()
{
	Dispatch([&]
	{
		auto* editor = std::exchange(m_Impl->m_Editor, nullptr);
		++m_Impl->m_EditVersion;
		m_Impl->m_EditingItem = 0; m_Impl->m_EditingColumn = 0;
		if (!editor) return;
		const bool focused = editor->HasFocus();
		editor->SetVisible(false);
		RemoveChild(*editor);
		if (focused && IsVisible() && IsEnabled()) RequestFocus();
		InvalidatePaint();
	});
}
}
