#include "UI/WidgetTree.hpp"
#include <catch2/catch_test_macros.hpp>

namespace
{
class RemovingWidget : public Yukiri::Widget
{
public:
	explicit RemovingWidget(bool& destroyed) : m_Destroyed(destroyed) { SetFocusable(true); }
	~RemovingWidget() override { m_Destroyed = true; }
	void OnPointerDown(Yukiri::PointerEvent&) override
	{
		CapturePointer();
		GetParent()->RemoveChild(*this);
		REQUIRE_FALSE(m_Destroyed);
	}
private:
	bool& m_Destroyed;
};
class Parent : public Yukiri::Widget
{
public:
	void OnPointerDown(Yukiri::PointerEvent&) override { ++m_Count; }
	int m_Count = 0;
};
}

TEST_CASE("Event removal waits for bubbling and clears observers", "[tree]")
{
	bool destroyed = false;
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Parent>();
	auto* parent = root.get();
	root->AddChild<RemovingWidget>(destroyed);
	tree.SetRoot(std::move(root));
	tree.Layout({100, 100});
	tree.PointerDown({{20, 20}, Yukiri::PointerButton::Left});
	REQUIRE(destroyed);
	REQUIRE(parent->m_Count == 1);
	REQUIRE(parent->GetChildren().empty());
	REQUIRE(tree.m_Focus == nullptr);
	REQUIRE(tree.m_Capture == nullptr);
}

TEST_CASE("Tab focus skips disabled and hidden subtrees", "[tree]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	auto& first = root->AddChild<Yukiri::Widget>();
	first.SetFocusable(true);
	auto& second = root->AddChild<Yukiri::Widget>();
	second.SetFocusable(true);
	second.SetEnabled(false);
	auto& third = root->AddChild<Yukiri::Widget>();
	third.SetFocusable(true);
	tree.SetRoot(std::move(root));
	tree.KeyDown({Yukiri::Key::Tab});
	REQUIRE(tree.m_Focus == &first);
	tree.KeyDown({Yukiri::Key::Tab});
	REQUIRE(tree.m_Focus == &third);
	tree.KeyDown({Yukiri::Key::Tab, {true, false, false}});
	REQUIRE(tree.m_Focus == &first);
	first.SetVisible(false);
	REQUIRE(tree.m_Focus == nullptr);
}

TEST_CASE("Layout callbacks defer destruction until traversal completes", "[tree]")
{
	class RemovingMeasure : public Yukiri::Widget
	{
	public:
		explicit RemovingMeasure(bool& destroyed) : m_Destroyed(destroyed) {}
		~RemovingMeasure() override { m_Destroyed = true; }
	protected:
		Yukiri::Size OnMeasure(Yukiri::Size) override
		{
			GetParent()->RemoveChild(*this);
			REQUIRE_FALSE(m_Destroyed);
			return {20, 20};
		}
	private:
		bool& m_Destroyed;
	};
	bool destroyed = false;
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	root->AddChild<RemovingMeasure>(destroyed);
	tree.SetRoot(std::move(root));
	tree.Layout({100, 100});
	REQUIRE(destroyed);
	REQUIRE(tree.m_Root->GetChildren().empty());
	REQUIRE(tree.m_LayoutDirty);
}

TEST_CASE("Direct focus and visibility callbacks can remove their own widgets", "[tree]")
{
	class RemovingFocus : public Yukiri::Widget
	{
	public:
		RemovingFocus(bool& destroyed, bool removeOnGain) : m_Destroyed(destroyed), m_RemoveOnGain(removeOnGain) { SetFocusable(true); }
		~RemovingFocus() override { m_Destroyed = true; }
		void OnFocusChanged(bool focused) override
		{
			if (focused != m_RemoveOnGain) return;
			GetParent()->RemoveChild(*this);
			REQUIRE_FALSE(m_Destroyed);
		}
	private:
		bool& m_Destroyed;
		bool m_RemoveOnGain;
	};
	bool destroyed = false;
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	auto* parent = root.get();
	auto& gain = root->AddChild<RemovingFocus>(destroyed, true);
	tree.SetRoot(std::move(root));
	gain.RequestFocus();
	REQUIRE(destroyed);
	REQUIRE(tree.m_Focus == nullptr);
	REQUIRE(parent->GetChildren().empty());
	for (bool hide : {true, false})
	{
		destroyed = false;
		auto& loss = parent->AddChild<RemovingFocus>(destroyed, false);
		loss.RequestFocus();
		if (hide) loss.SetVisible(false); else loss.SetEnabled(false);
		REQUIRE(destroyed);
		REQUIRE(tree.m_Focus == nullptr);
		REQUIRE(parent->GetChildren().empty());
	}
}
