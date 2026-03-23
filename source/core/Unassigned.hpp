#pragma once

#include <cstdint>

/** \file Unassigned.hpp */

namespace entt {
	enum class entity : uint32_t;
}

namespace apollo {
	/**
	 * \brief Represents an object which doesn't have a "valid" value. Useful for resource handles
	 * and such
	 * \details Valid specializations of this template should declare a static constant of type \b T
	 * named Value.
	 */
	template <class T>
	struct UnassignedT;

	template <class T>
	struct UnassignedT<T*>
	{
		static constexpr T* Value = nullptr;
	};

	template <>
	struct UnassignedT<entt::entity>
	{
		static constexpr entt::entity Value = entt::entity(UINT32_MAX);
	};

	/**
	 * \brief Shortcut constant to access the unassigned value for a given type
	 */
	template <class T>
	static constexpr T Unassigned = UnassignedT<T>::Value;
} // namespace apollo