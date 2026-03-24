#pragma once

#include <PCH.hpp>
#include <entt/entity/fwd.hpp>
#include <string_view>

/** \file Reflection.hpp */

namespace apollo::ecs {
	/** \brief Reflection struct: gives static information about a component type

	* This is used for  serialization/deserialization and editor UI widgets
	 * \tparam Ptr: A list of pointers to the members of your component type
	 * \sa apollo::TransformComponent for an example of how to use this.
	 */
	template <auto... Ptr>
	requires(std::is_member_object_pointer_v<decltype(Ptr)>&&...) struct ComponentReflection
	{
		const char* m_ComponentName = nullptr;
		/**
		 * \brief The names of all relevant fields
		 * \note You don't have to declare all your data members in there: just the ones required to
		 * load/unload/display your component.
		 */
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

	/**
	 * \brief Checks whether an object type is a valid ECS component.
	 *
	 * \details A Component type must declare an accessible static object called Reflection.
	 * The type of this object must be a specialization of apollo::ecs::ComponentReflection.
	 */
	template <class C>
	concept Component = _internal::IsComponentReflection<decltype(C::Reflection)>::value;

	/**
	 * \brief Runtime component information, stored in the \ref apollo::ecs::ComponentRegistry
	 * "Component Registry"
	 */
	struct ComponentInfo
	{
		std::string_view m_Name;
		bool (*m_Deserialize)(entt::entity entity, entt::registry& world, const void* data) =
			nullptr;
	};
} // namespace apollo::ecs