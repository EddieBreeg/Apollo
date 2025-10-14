#include <catch2/catch_test_macros.hpp>
#include <core/GameTime.hpp>
#include <ecs/System.hpp>
#include <entt/entity/registry.hpp>

namespace {
	struct S1
	{
		int32 value = 0;

		void Update(entt::registry&, const apollo::GameTime&) { ++value; }
	};

	struct S2
	{
		uint32& m_OnDelete;
		~S2() { ++m_OnDelete; }
		void Update(entt::registry&, const apollo::GameTime&) {}
	};
} // namespace

namespace apollo::ecs::ut {
	TEST_CASE("Create system instance", "[ecs]")
	{
		SystemInstance system = SystemInstance::Create<S1>();
		CHECK(system.GetAs<const S1>()->value == 0);
	}

	TEST_CASE("System update", "[ecs]")
	{
		SystemInstance system = SystemInstance::Create<S1>();
		REQUIRE(system.GetAs<const S1>()->value == 0);

		entt::registry world;
		apollo::GameTime time;
		system.Update(world, time);
		CHECK(system.GetAs<const S1>()->value == 1);
	}

	TEST_CASE("System move", "[ecs]")
	{
		SystemInstance s0 = SystemInstance::Create<S1>(1);
		SystemInstance s1{ std::move(s0) };

		CHECK_FALSE(s0);
		CHECK(s1);
		CHECK(s1.GetAs<const S1>()->value == 1);

		s0 = std::move(s1);

		CHECK_FALSE(s1);
		CHECK(s0);
		CHECK(s0.GetAs<const S1>()->value == 1);
	}

	TEST_CASE("System destruction")
	{
		uint32 deleteCount = 0;
		{
			SystemInstance s = SystemInstance::Create<S2>(deleteCount);
			s.Shutdown();
			CHECK(deleteCount == 1);
		}
		{
			SystemInstance s = SystemInstance::Create<S2>(deleteCount);
			(void)s;
		}
		CHECK(deleteCount == 2);
	}
} // namespace apollo::ecs::ut
