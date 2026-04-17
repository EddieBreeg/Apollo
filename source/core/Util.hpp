#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

/** \file Util.hpp */

namespace apollo {
	/** \name Swap function
	 * \brief Swaps two objects
	 * \tparam T: The object type
	 * \details The correct overload will automatically be deduced. If \b T declares a method called
	 * either swap or Swap, this method will be called. Otherwise if \b T is trivially move
	 * assignable, a classic swap will be applied using a temp variable.
	 * @{ */

	template <class T>
	constexpr void Swap(T& a, T& b) noexcept(noexcept(a.swap(b))) requires(requires { a.swap(b); })
	{
		a.swap(b);
	}
	template <class T>
	constexpr void Swap(T& a, T& b) noexcept(noexcept(a.Swap(b))) requires(requires { a.Swap(b); })
	{
		a.Swap(b);
	}

	template <class T>
	constexpr void Swap(T& a, T& b) noexcept requires(
		std::is_trivially_move_assignable_v<T> &&
		!(requires { a.swap(b); } || requires { a.Swap(b); }))
	{
		T temp{ std::move(a) };
		a = std::move(b);
		b = std::move(temp);
	}

	template <class T, size_t N>
	constexpr void Swap(T (&a)[N], T (&b)[N]) noexcept(
		noexcept(Swap(std::declval<T&>(), std::declval<T&>())))
		requires(requires(T& left, T& right) { Swap(left, right); })
	{
		for (size_t i = 0; i < N; ++i)
			Swap(a[i], b[i]);
	}

	/** @} */

	template <class T>
	concept Swappable = requires(T & a, T& b)
	{
		{ Swap(a, b) };
	};

	template <class T>
	concept NoThrowSwappable = noexcept(Swap(std::declval<T&>(), std::declval<T&>()));

	struct PointerDiff
	{
		template <class T>
		constexpr PointerDiff(T&& val) noexcept
			: m_Value(static_cast<int64_t>(val))
		{}
		int64_t m_Value;
		[[nodiscard]] constexpr operator int64_t() const noexcept { return m_Value; }
	};

	[[nodiscard]] inline void* operator+(void* p, PointerDiff diff) noexcept
	{
		return static_cast<std::byte*>(p) + diff.m_Value;
	}
	[[nodiscard]] inline void* operator-(void* p, PointerDiff diff) noexcept
	{
		return static_cast<std::byte*>(p) - diff.m_Value;
	}
} // namespace apollo