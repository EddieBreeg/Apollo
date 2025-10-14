#pragma once

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

	template <class T>
	static constexpr T Unassigned = UnassignedT<T>::Value;
} // namespace apollo