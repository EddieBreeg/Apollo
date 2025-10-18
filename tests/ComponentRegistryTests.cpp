#include <catch2/catch_test_macros.hpp>
#include <ecs/ComponentRegistry.hpp>

#define COMPONENT_JSON_TEST(name) TEST_CASE(name, "[ecs][component][scene]")

namespace apollo::ecs::ut {
	struct C1
	{
		int32 m_Value = 0;
		std::string_view m_Str;

		static constexpr ComponentReflection<&C1::m_Value, &C1::m_Str> Reflection{
			"C1",
			{ "value", "str" },
		};
	};

	struct Helper
	{
		ComponentRegistry* m_Registry = &ComponentRegistry::Init();
		entt::registry m_World;
		~Helper() { ComponentRegistry::Shutdown(); }
	};

	static_assert(Component<C1>);

	COMPONENT_JSON_TEST("Load C1 From Json")
	{
		C1 comp;
		const nlohmann::json j{
			{ "value", 1 },
			{ "str", "hello" },
		};
		CHECK(json::Converter<C1>::FromJson(comp, j));
		CHECK(comp.m_Value == 1);
		CHECK((comp.m_Str == "hello"));
	}

	COMPONENT_JSON_TEST("Fail to Load C1 From Json")
	{
		C1 comp;
		const nlohmann::json j{};
		CHECK(!json::Converter<C1>::FromJson(comp, j));
		CHECK(comp.m_Value == 0);
		CHECK((comp.m_Str == std::string_view{}));
	}

	COMPONENT_JSON_TEST("Register Component")
	{
		Helper helper;
		const ComponentInfo& info = helper.m_Registry->RegisterComponent<C1>();
		CHECK((info.m_Name == C1::Reflection.m_ComponentName));

		const entt::entity e = helper.m_World.create();
		const nlohmann::json j{
			{ "value", 66 },
			{ "str", "text" },
		};
		REQUIRE(info.m_Deserialize);
		CHECK(info.m_Deserialize(e, helper.m_World, &j));

		const C1* comp = helper.m_World.try_get<const C1>(e);
		REQUIRE(comp);
		CHECK(comp->m_Value == 66);
		CHECK((comp->m_Str == "text"));

		const ComponentInfo* ptr = helper.m_Registry->GetInfo<C1>();
		CHECK(ptr == &info);
	}
} // namespace apollo::ecs::ut