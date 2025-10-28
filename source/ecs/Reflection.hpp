#pragma once

#include <PCH.hpp>
#include <entt/entity/fwd.hpp>
#include <string_view>

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