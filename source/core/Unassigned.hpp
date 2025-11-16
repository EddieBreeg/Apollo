#pragma once

#include <cstdint>

namespace entt {
	enum class entity : uint32_t;
}

namespace apollo {
	/*
	 * Represents an object which doesn't have a "valid" value. Useful for resource handles
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

	template <class T>
	static constexpr T Unassigned = UnassignedT<T>::Value;
} // namespace apollo