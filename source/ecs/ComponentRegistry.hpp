#pragma once

/** \file ComponentRegistry.hpp */

#include <PCH.hpp>
#include <core/HashedString.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <core/Map.hpp>
#include <core/Singleton.hpp>
#include <entt/entity/registry.hpp>

#include "Reflection.hpp"

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
		return ((Visit(out_comp.*M, json, reflection.m_Fields[index++].GetString())) && ...);
	}
};

namespace apollo::ecs {
	/**
	 * \brief Holds runtime information about Component types
	 * \details Components must be registered in here when runtime reflection is required.
	 * This is typically so we can load them from scene files or display them in UI widgets.
	 * If your component type doesn't need these features, then registering isn't required.
	 * \sa Component requirements
	 */
	class ComponentRegistry : public Singleton<ComponentRegistry>
	{
	public:
		/**
		 * \brief Generates runtime information about the provided component type and adds it to the
		 * internal map
		 */
		template <Component C>
		const ComponentInfo& RegisterComponent()
		{
			const auto res = m_InfoMap.try_emplace(C::Reflection.m_ComponentName, CreateInfo<C>());
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
			const auto it = m_InfoMap.find(C::Reflection.m_ComponentName);
			return it == m_InfoMap.end() ? nullptr : &it->second;
		}

		const ComponentInfo* GetInfo(const HashedString& name) const noexcept
		{
			const auto it = m_InfoMap.find(name);
			return it == m_InfoMap.end() ? nullptr : &it->second;
		}

	private:
		ComponentRegistry() = default;
		friend class Singleton<ComponentRegistry>;

		HashedStringMap<ComponentInfo> m_InfoMap;

		static inline std::unique_ptr<ComponentRegistry> s_Instance;

		template <Component C>
		static constexpr ComponentInfo CreateInfo()
		{
			return ComponentInfo{
				.m_Name = C::Reflection.m_ComponentName.GetString(),
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