#include <Yukiri/Core/Detail/Transition.hpp>
#include "UI/WidgetTree.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

TEST_CASE("Transitions retarget continuously and finish independently of frame rate", "[motion]")
{
	Yukiri::Detail::Transition transition;
	transition.Retarget(1, 100ms);
	REQUIRE(transition.Advance(50ms));
	REQUIRE(transition.GetValue() == Catch::Approx(0.875f));
	transition.Retarget(0, 100ms);
	transition.Advance(0ns);
	REQUIRE(transition.GetValue() == Catch::Approx(0.875f));
	REQUIRE_FALSE(transition.Advance(100ms));
	REQUIRE(transition.GetValue() == 0);
	transition.Retarget(1, -1ms);
	REQUIRE(transition.GetValue() == 1);
	for (int rate : {60, 120, 144, 165, 240})
	{
		transition.Snap(0);
		transition.Retarget(1, 160ms);
		const auto step = std::chrono::nanoseconds(1'000'000'000 / rate);
		for (auto elapsed = 0ns; elapsed < 160ms; elapsed += step) transition.Advance(step);
		REQUIRE(transition.GetValue() == 1);
		REQUIRE_FALSE(transition.IsActive());
	}
}

TEST_CASE("Disabled motion snaps and hidden widgets stop animation scheduling", "[motion]")
{
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	tree.SetRoot(std::move(root));
	tree.Layout({100, 100});
	tree.m_Theme.m_Motion.m_Enabled = false;
	tree.PointerMove({{10, 10}});
	tree.Tick();
	REQUIRE_FALSE(tree.m_Animating);
	tree.m_Theme.m_Motion.m_Enabled = true;
	tree.PointerMove({{-1, -1}});
	tree.Tick();
	REQUIRE(tree.m_Animating);
	tree.m_Root->SetVisible(false);
	tree.Tick();
	REQUIRE_FALSE(tree.m_Animating);
}

TEST_CASE("Animation callbacks may destroy their own subtree safely", "[motion]")
{
	class AnimatedRemoval : public Yukiri::Widget
	{
	public:
		explicit AnimatedRemoval(bool& destroyed) : m_Destroyed(destroyed) {}
		~AnimatedRemoval() override { m_Destroyed = true; }
		void Start() { m_Remove = true; RequestAnimationFrame(); }
	protected:
		bool OnAnimate(std::chrono::nanoseconds) override
		{
			if (m_Remove)
			{
				GetParent()->RemoveChild(*this);
				REQUIRE_FALSE(m_Destroyed);
			}
			return false;
		}
	private:
		bool& m_Destroyed;
		bool m_Remove = false;
	};
	bool destroyed = false;
	Yukiri::Detail::WidgetTree tree;
	auto root = std::make_unique<Yukiri::Widget>();
	auto& animated = root->AddChild<AnimatedRemoval>(destroyed);
	tree.SetRoot(std::move(root));
	tree.Layout({100, 100});
	animated.Start();
	tree.Tick();
	REQUIRE(destroyed);
	REQUIRE(tree.m_Root->GetChildren().empty());
}
