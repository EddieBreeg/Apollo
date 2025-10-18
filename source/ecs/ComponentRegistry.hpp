#pragma once

// This is not part of the core API! This is used to have ECS component
// reflection, which depends on whether we're in a game or editor context

// #if defined(APOLLO_GAME_ENTRY) || defined(APOLLO_EDITOR)

#include <PCH.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <core/Map.hpp>
#include <core/Singleton.hpp>
#include <core/StringHash.hpp>
#include <entt/entity/registry.hpp>

namespace apollo::ecs {
	/* Reflection struct: gives static information about a component type. Used for
	 * serialization/deserialization and editor UI
	 * \tparam Ptr: A list of pointers to the members of your component type
	 */
	template <auto... Ptr>
	requires(std::is_member_object_pointer_v<decltype(Ptr)>&&...) struct ComponentReflection
	{
		const char* m_ComponentName = nullptr;
		const char* m_Fields[sizeof...(Ptr)];
	};

	namespace _internal {
		template <class T>
		struct IsComponentReflection : std::false_type
		{};
		template <auto... Ptr>
		struct IsComponentReflection<ComponentReflection<Ptr...>> : std::true_type
		{};
		template <class T>
		struct IsComponentReflection<const T> : IsComponentReflection<T>
		{};
	} // namespace _internal

	/*
	 * Pretty much the only requirement for a type to be a component is to declare a
	 * static accessible reflection object called Reflection
	 */
	template <class C>
	concept Component = _internal::IsComponentReflection<decltype(C::Reflection)>::value;

	struct ComponentInfo
	{
		std::string_view m_Name;
		bool (*m_Deserialize)(entt::entity entity, entt::registry& world, const void* data) =
			nullptr;
	};
} // namespace apollo::ecs

// #ifdef APOLLO_EDITOR

template <apollo::ecs::Component C>
struct apollo::json::Converter<C>
{
	static bool FromJson(C& out_comp, const nlohmann::json& json)
	{
		return FromJson(out_comp, C::Reflection, json);
	}

private:
	template <auto... M>
	static bool FromJson(
		C& out_comp,
		const ecs::ComponentReflection<M...>& reflection,
		const nlohmann::json& json)
	{
		uint32 index = 0;
		return ((Visit(out_comp.*M, json, reflection.m_Fields[index++])) && ...);
	}
};

// #endif

namespace apollo::ecs {
	class ComponentRegistry : public Singleton<ComponentRegistry>
	{
	public:
		template <Component C>
		const ComponentInfo& RegisterComponent()
		{
			const auto res = m_InfoMap.try_emplace(
				StringHash{ C::Reflection.m_ComponentName },
				CreateInfo<C>());
			if (!res.second)
			{
				APOLLO_LOG_ERROR(
					"Component {} was already registered",
					C::Reflection.m_ComponentName);
			}
			return res.first->second;
		}

		template <Component C>
		const ComponentInfo* GetInfo() const noexcept
		{
			const auto it = m_InfoMap.find(StringHash{ C::Reflection.m_ComponentName });
			return it == m_InfoMap.end() ? nullptr : &it->second;
		}

		const ComponentInfo* GetInfo(std::string_view name) const noexcept
		{
			const auto it = m_InfoMap.find(StringHash{ name });
			return it == m_InfoMap.end() ? nullptr : &it->second;
		}

	private:
		ComponentRegistry() = default;
		friend class Singleton<ComponentRegistry>;

		HashMap<StringHash, ComponentInfo> m_InfoMap;

		static inline std::unique_ptr<ComponentRegistry> s_Instance;

		template <Component C>
		static constexpr ComponentInfo CreateInfo()
		{
			return ComponentInfo{
				.m_Name = C::Reflection.m_ComponentName,
				.m_Deserialize =
					[](entt::entity e, entt::registry& world, const void* data)
				{
					C comp{};
					if (!json::Converter<C>::FromJson(
							comp,
							*static_cast<const nlohmann::json*>(data)))
					{
						APOLLO_LOG_ERROR(
							"Failed to load component {}",
							C::Reflection.m_ComponentName);
						return false;
					}
					world.emplace<C>(e, std::move(comp));
					return true;
				},
			};
		}
	};
} // namespace apollo::ecs
// #endif