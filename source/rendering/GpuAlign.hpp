#pragma once

#include <PCH.hpp>

namespace apollo::rdr {
	namespace _internal {
		template <class T>
		struct GpuAlignment
		{
			static constexpr uint32 Value = sizeof(T);
		};

		template <class T>
		struct GpuAlignment<glm::vec<3, T>>
		{
			static constexpr uint32 Value = 4 * sizeof(T);
		};
	} // namespace _internal

#define GPU_ALIGN(T) alignas(apollo::rdr::_internal::GpuAlignment<T>::Value) T
} // namespace apollo::rdr