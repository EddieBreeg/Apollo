#pragma once

#include <PCH.hpp>

/** \file StaticStorage.hpp */

namespace apollo {
	/**
	 * \brief Fixed-size storage utility
	 */
	template <uint32 Size, uint32 Alignment = alignof(std::max_align_t)>
	struct StaticStorage
	{
		alignas(Alignment) std::byte m_Buf[Size];

		/** \name GetAs()
		 * \brief Reinterprets the bytes in the buffer.
		 * \warning The conversion is not type safe, use this carefully.
		 * @{ */
		template <class T>
		[[nodiscard]] T* GetAs() noexcept requires(alignof(T) <= Alignment && sizeof(T) <= Size)
		{
			return reinterpret_cast<T*>(m_Buf);
		}

		template <class T>
		[[nodiscard]] const T* GetAs() const noexcept
			requires(alignof(T) <= Alignment && sizeof(T) <= Size)
		{
			return reinterpret_cast<const T*>(m_Buf);
		}
		/** @} */

		/**
		 * \brief Uses placement-new to store a new value
		 * \warning If an object was previously stored in the buffer, its data is overwritten and
		 * the destructor is not called. This invokes undefined behaviour if said object is not
		 * trivially destructible.
		 */
		template <class T>
		T* Construct(auto&&... args) noexcept(std::is_nothrow_constructible_v<T, decltype(args)...>)
			requires(
				std::is_constructible_v<T, decltype(args)...> && sizeof(T) <= Size &&
				alignof(T) <= Alignment)
		{
			return new (m_Buf) T{ std::forward<decltype(args)>(args)... };
		}

		void Swap(StaticStorage& other) noexcept { std::swap(m_Buf, other.m_Buf); }
	};
} // namespace apollo