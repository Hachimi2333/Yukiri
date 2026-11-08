#pragma once

#include <Yukiri/Core/Error.hpp>
#include <Yukiri/UI/Widget.hpp>
#include <string>

namespace Yukiri
{
class TabView : public Widget
{
public:
	TabView();
	Widget& AddTab(std::string title, std::unique_ptr<Widget> content);
	[[nodiscard]] Result<void> SetSelectedIndex(std::size_t index);
	[[nodiscard]] std::size_t GetSelectedIndex() const noexcept;
	void OnPointerDown(PointerEvent& event) override;
	void OnKeyDown(KeyEvent& event) override;
	Signal<std::size_t> m_SelectionChanged;
protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
	void OnPaint(Canvas& canvas) override;
private:
	[[nodiscard]] std::vector<Rect> Headers() const;
	std::vector<std::string> m_Titles;
	std::size_t m_Selected = 0;
};
}
