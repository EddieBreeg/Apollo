#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace apollo {
	namespace _internal {
		template <class T, class = void>
		struct SwapTraits
		{
			static constexpr bool HasSwapMethod = false;
			static constexpr bool IsNoexcept = false;
		};

		template <class T>
		struct SwapTraits<
			T,
			std::enable_if_t<std::is_void_v<decltype(std::declval<T>().Swap(std::declval<T&>()))>>>
		{
			static constexpr bool HasSwapMethod = true;
			static constexpr void (T::*Func)(T&) = &T::Swap;
			static constexpr bool IsNoexcept = noexcept(std::declval<T>().Swap(std::declval<T&>()));
		};

		template <class T>
		struct SwapTraits<
			T,
			std::enable_if_t<std::is_void_v<decltype(std::declval<T>().swap(std::declval<T&>()))>>>
		{
			static constexpr bool HasSwapMethod = true;
			static constexpr void (T::*Func)(T&) = &T::swap;
			static constexpr bool IsNoexcept = noexcept(std::declval<T>().swap(std::declval<T&>()));
		};
	} // namespace _internal

	template <class T>
	constexpr void Swap(T& a, T& b) noexcept(_internal::SwapTraits<T>::IsNoexcept)
		requires(_internal::SwapTraits<T>::HasSwapMethod)
	{
		(a.*_internal::SwapTraits<T>::Func)(b);
	}

	template <class T>
	constexpr void Swap(T& a, T& b) noexcept
		requires(std::is_trivially_move_assignable_v<T> && !_internal::SwapTraits<T>::HasSwapMethod)
	{
		T temp{ std::move(a) };
		a = std::move(b);
		b = std::move(temp);
	}

	template <class T>
	concept Swappable = requires(T & a, T& b)
	{
		{ Swap(a, b) };
	};

	template <class T>
	concept NoThrowSwappable = noexcept(Swap(std::declval<T&>(), std::declval<T&>()));

	template <Swappable T, size_t N>
	constexpr void Swap(T (&a)[N], T (&b)[N]) noexcept(NoThrowSwappable<T>)
	{
		for (size_t i = 0; i < N; ++i)
			Swap(a[i], b[i]);
	}
} // namespace apollo