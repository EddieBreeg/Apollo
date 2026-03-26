#pragma once

#include <PCH.hpp>

/** \file GpuAlign.hpp */

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

/**
 * \def GPU_ALIGN(T)
 * \brief Constrains the alignment of a type to match the value used on the GPU.
 * \details This is used on struct members where the data is meant to be uploaded to a GPU buffer.
 */
#define GPU_ALIGN(T) alignas(apollo::rdr::_internal::GpuAlignment<T>::Value) T
} // namespace apollo::rdr