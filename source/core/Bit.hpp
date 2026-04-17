#pragma once

/** \file Bit.hpp */

#include <PCH.hpp>
#include <bit>

namespace apollo {
	/// Iterator over bit spans
	/// \tparam T: An unsigned integral type, which may be const qualified
	template <class T>
	requires(std::unsigned_integral<std::remove_cv_t<T>>) struct BitIterator
	{
		struct reference
		{
			T* m_Ptr = nullptr;
			size_t m_Pos = 0;

			constexpr reference& operator=(bool value) requires(!std::is_const_v<T>)
			{
				const T bit = 1ull << m_Pos;
				*m_Ptr = value ? (*m_Ptr | bit) : (*m_Ptr & ~bit);
				return *this;
			}
			constexpr operator bool() const { return *m_Ptr & (1ull << m_Pos); }
		};

		constexpr BitIterator() = default;
		/// \brief Creates an iterator from an address and an offset
		/// \param ptr: The address the iterator points to
		/// \param n: The bit-offset from the provided pointer. 0 would mean the right-most bit in
		/// the integer value
		explicit constexpr BitIterator(T* ptr, size_t n = 0)
			: m_Ptr(ptr)
			, m_Pos(n)
		{}

		[[nodiscard]] constexpr bool operator*() const requires(std::is_const_v<T>)
		{
			constexpr T mask = ~T{ 0 };
			return *m_Ptr & (1ull << (m_Pos & mask));
		}
		[[nodiscard]] constexpr reference operator*() const noexcept requires(!std::is_const_v<T>)
		{
			return reference{ m_Ptr, m_Pos };
		}

		constexpr BitIterator& operator++() noexcept
		{
			constexpr size_t m = 8 * sizeof(T);
			m_Pos = (++m_Pos) % m;
			m_Ptr += !m_Pos;
			return *this;
		}
		constexpr BitIterator operator++(int) noexcept
		{
			const BitIterator res{ m_Ptr, m_Pos };
			constexpr size_t m = 8 * sizeof(T);
			m_Pos = (++m_Pos) % m;
			m_Ptr += !m_Pos;
			return res;
		}

		template <class U>
		[[nodiscard]] constexpr bool operator==(const BitIterator<U>& other) const noexcept
		{
			return m_Ptr == other.m_Ptr && m_Pos == other.m_Pos;
		}
		template <class U>
		[[nodiscard]] constexpr bool operator!=(const BitIterator<U>& other) const noexcept
		{
			return m_Ptr != other.m_Ptr || m_Pos != other.m_Pos;
		}

		constexpr BitIterator& Set()
		{
			*m_Ptr |= T{ 1 } << m_Pos;
			return *this;
		}
		constexpr BitIterator& Clear()
		{
			*m_Ptr &= ~(T{ 1 } << m_Pos);
			return *this;
		}
		constexpr const BitIterator& Set() const
		{
			*m_Ptr |= (T{ 1 } << m_Pos);
			return *this;
		}
		constexpr const BitIterator& Clear() const
		{
			*m_Ptr &= ~(T{ 1 } << m_Pos);
			return *this;
		}

		[[nodiscard]] T* GetPtr() const noexcept { return m_Ptr; }

	private:
		T* m_Ptr = nullptr;
		size_t m_Pos = 0;
	};

	/**
	 * \brief Allows easy manipulation of contiguous bit buffers
	 */
	template <std::unsigned_integral T>
	struct BitSpan
	{
		constexpr BitSpan() = default;
		/// \brief Construct from a pointer and a size
		/// \param start: The address of the first bit the span points to
		/// \param n: The span size, **in bits**
		constexpr BitSpan(T* start, size_t n) noexcept
			: m_Start(start)
			, m_Size(n)
		{}
		constexpr BitSpan(T* start, T* end) noexcept
			: m_Start(start)
			, m_Size((end - start) * 8 * sizeof(T))
		{}

		constexpr BitSpan(const BitSpan<T>& other) = default;
		constexpr BitSpan& operator=(const BitSpan<T>& other) = default;
		constexpr ~BitSpan() = default;

		[[nodiscard]] constexpr BitIterator<T> begin() const noexcept
		{
			return BitIterator<T>{ m_Start };
		}
		[[nodiscard]] constexpr BitIterator<T> end() const noexcept
		{
			constexpr size_t m = 8 * sizeof(T);
			return BitIterator<T>{
				m_Start + m_Size / m,
				m_Size % m,
			};
		}

		/// \brief The size of the span, in bits
		/// \note The result is always a multiple of `8 * sizeof(T)`
		[[nodiscard]] constexpr size_t GetSize() const noexcept { return m_Size; }

		constexpr BitIterator<T>::reference operator[](size_t n) const noexcept
			requires(!std::is_const_v<T>)
		{
			constexpr size_t m = 8 * sizeof(T);
			using RefT = BitIterator<T>::reference;
			return RefT{
				m_Start + n / m,
				n % m,
			};
		}
		[[nodiscard]] constexpr bool operator[](size_t n) const requires(std::is_const_v<T>)
		{
			constexpr size_t m = 8 * sizeof(T);
			T* const it = m_Start + n / m;
			return *it & (1u << (n % m));
		}

	private:
		T* m_Start = nullptr;
		size_t m_Size = 0;
	};
} // namespace apollo