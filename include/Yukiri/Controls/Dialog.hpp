#pragma once

#include <Yukiri/UI/Widget.hpp>
#include <Yukiri/UI/CloseRequestEvent.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Yukiri
{
class Grid;
class WrapPanel;
class Label;
class Button;

class Dialog : public Widget
{
public:
	enum class Result { Cancel, Ok, Yes, No, Save, Discard };
	explicit Dialog(std::string title = {});
	[[nodiscard]] static Yukiri::Result<Dialog*> Show(Window& window, std::unique_ptr<Dialog> dialog);
	void SetTitle(std::string_view title);
	Widget& SetContent(std::unique_ptr<Widget> content);
	void SetButtons(std::vector<std::pair<std::string, Result>> buttons, Result defaultResult = Result::Ok,
		std::optional<Result> cancelResult = Result::Cancel);
	[[nodiscard]] Widget* GetContent() const noexcept;
	[[nodiscard]] Button* GetButton(Result result) const noexcept;
	void Complete(Result result);
	void OnPreviewKeyDown(KeyEvent& event) override;
	Signal<Result> m_Completed;
	Signal<Result, CloseRequestEvent&> m_CloseRequested;
protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
	void OnPaint(Canvas& canvas) override;
private:
	void CompleteImpl(Result result, bool force);
	Grid* m_Layout = nullptr;
	Label* m_Title = nullptr;
	WrapPanel* m_ButtonPanel = nullptr;
	Widget* m_Content = nullptr;
	std::weak_ptr<int> m_ContentLifetime;
	struct Action { Button* m_Button; std::weak_ptr<int> m_Lifetime; Result m_Result; };
	std::vector<Action> m_Actions;
	Result m_DefaultResult = Result::Ok;
	std::optional<Result> m_CancelResult = Result::Cancel;
	bool m_Complete = false;
	bool m_Completing = false;
};
}
