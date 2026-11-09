#include "UI/WidgetTree.hpp"
#include "Core/Retire.hpp"
#include <Yukiri/Controls/Button.hpp>
#include <Yukiri/Controls/Dialog.hpp>
#include <Yukiri/Controls/TextBox.hpp>
#include <Yukiri/Layout/Border.hpp>
#include <Yukiri/Layout/StackPanel.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Modal content traps navigation and restores live input focus", "[popup][focus]")
{
	Yukiri::Detail::WidgetTree tree;
	tree.m_Theme.m_Motion.m_Enabled = false;
	auto root = std::make_unique<Yukiri::StackPanel>();
	auto& outside = root->AddChild<Yukiri::Button>("Outside");
	tree.SetRoot(std::move(root));
	tree.Layout({600, 400});
	tree.SetFocus(&outside);
	auto modal = std::make_unique<Yukiri::StackPanel>();
	auto& first = modal->AddChild<Yukiri::Button>("First");
	auto& second = modal->AddChild<Yukiri::Button>("Second");
	int canceled = 0;
	auto opened = tree.OpenPopup(nullptr, std::move(modal), {}, true, false, true, [&] { ++canceled; });
	REQUIRE(opened);
	tree.Layout({600, 400});
	REQUIRE(first.HasFocus());
	outside.RequestFocus();
	REQUIRE(first.HasFocus());
	tree.KeyDown({Yukiri::Key::Tab});
	REQUIRE(second.HasFocus());
	tree.KeyDown({Yukiri::Key::Tab});
	REQUIRE(first.HasFocus());
	tree.PointerDown({{0, 0}, Yukiri::PointerButton::Left});
	REQUIRE(first.HasFocus());
	tree.ClosePopupFor(*opened, true);
	REQUIRE(outside.HasFocus());
	REQUIRE(canceled == 1);
	tree.CloseAllPopups();
	REQUIRE(canceled == 1);
}

TEST_CASE("Retiring an old popup branch does not dismiss an unrelated new modal", "[popup][lifetime]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	auto* body = root.get();
	auto& owner = root->AddChild<Yukiri::Button>("Owner");
	tree.SetRoot(std::move(root));
	tree.Layout({600, 400});
	auto old = tree.OpenPopup(&owner, std::make_unique<Yukiri::Button>("Menu"), {0, 0, 50, 30});
	REQUIRE(old);
	tree.Layout({600, 400});
	tree.ClosePopupFor(*old);
	auto modal = tree.OpenPopup(nullptr, std::make_unique<Yukiri::Button>("Modal"), {}, true);
	REQUIRE(modal);
	body->RemoveChild(owner);
	REQUIRE(tree.HasPopup(*modal));
	REQUIRE(tree.GetModalRoot() == *modal);
	tree.CloseAllPopups();
}

TEST_CASE("Dialog defaults, validation veto, IME cancellation, and result delivery share safe dispatch", "[dialog]")
{
	Yukiri::Detail::WidgetTree tree;
	tree.m_Theme.m_Motion.m_Enabled = false;
	tree.SetRoot(std::make_unique<Yukiri::Widget>());
	tree.Layout({600, 400});
	auto dialog = std::make_unique<Yukiri::Dialog>("Form");
	auto* control = dialog.get();
	auto& editor = static_cast<Yukiri::TextBox&>(dialog->SetContent(std::make_unique<Yukiri::TextBox>("original")));
	bool allow = false;
	int results = 0;
	auto veto = dialog->m_CloseRequested.Connect([&](auto, auto& event) { event.m_Cancel = !allow; });
	auto completed = dialog->m_Completed.Connect([&](auto result) { REQUIRE(result == Yukiri::Dialog::Result::Ok); ++results; });
	REQUIRE(tree.OpenPopup(nullptr, std::move(dialog), {}, true));
	REQUIRE(editor.HasFocus());
	tree.KeyDown({Yukiri::Key::Enter});
	REQUIRE(tree.HasPopup(control));
	REQUIRE(results == 0);
	tree.TextEditing("ni", 2, 0);
	tree.KeyDown({Yukiri::Key::Escape});
	REQUIRE_FALSE(editor.IsComposing());
	REQUIRE(tree.HasPopup(control));
	allow = true;
	control->Complete(Yukiri::Dialog::Result::Ok);
	REQUIRE(results == 1);
	REQUIRE_FALSE(tree.OwnsPopup(control));
}

TEST_CASE("Popup focus callbacks may close and destroy their own content safely", "[popup][lifetime]")
{
	struct Closing : Yukiri::Button
	{
		bool& m_Destroyed;
		explicit Closing(bool& destroyed) : m_Destroyed(destroyed) {}
		~Closing() override { m_Destroyed = true; }
		void OnFocusChanged(bool focused) override
		{
			if (focused) { ClosePopup(true); REQUIRE_FALSE(m_Destroyed); }
		}
	};
	Yukiri::Detail::WidgetTree tree;
	tree.m_Theme.m_Motion.m_Enabled = false;
	bool destroyed = false;
	auto opened = tree.OpenPopup(nullptr, std::make_unique<Closing>(destroyed), {});
	REQUIRE_FALSE(opened);
	REQUIRE(destroyed);
}

TEST_CASE("Retired objects can append replacements after collection erasure", "[lifetime]")
{
	struct Object
	{
		std::function<void()> m_Destroy;
		~Object() { if (m_Destroy) m_Destroy(); }
	};
	std::vector<std::unique_ptr<Object>> objects;
	auto first = std::make_unique<Object>();
	first->m_Destroy = [&] { objects.push_back(std::make_unique<Object>()); };
	objects.push_back(std::move(first));
	Yukiri::Detail::RetireMatching(objects, [](const auto&) { return true; });
	REQUIRE(objects.size() == 1);
}
