#pragma once

#include <PCH.hpp>

#include "Assert.hpp"
#include <concepts>

namespace apollo {
	namespace _internal {
		template <class T>
		concept IntOrEnum = std::is_integral_v<T> || std::is_enum_v<T>;

		template <IntOrEnum T>
		static constexpr T SignBit = static_cast<T>(1ull << (8 * sizeof(T) - 1));

		template <class T>
		struct UnderlyingT
		{
			using Type = T;
		};

		template <IntOrEnum E>
		requires((std::is_enum_v<E>)) struct UnderlyingT<E>
		{
			using Type = std::underlying_type_t<E>;
		};

		template <class T>
		[[nodiscard]] constexpr auto ToUnderlying(T val) noexcept
		{
			return static_cast<typename UnderlyingT<T>::Type>(val);
		}

		template <class T>
		static constexpr bool IsSigned = std::is_signed_v<typename UnderlyingT<T>::Type>;

		template <class From, class To>
		struct LossyFloatConversion : std::false_type
		{};
		template <_internal::IntOrEnum From, std::floating_point To>
		requires(sizeof(From) >= sizeof(To)) struct LossyFloatConversion<From, To> : std::true_type
		{};
		template <std::floating_point From, IntOrEnum To>
		struct LossyFloatConversion<From, To> : std::true_type
		{};
		template <std::floating_point From, std::floating_point To>
		requires(sizeof(From) > sizeof(To)) struct LossyFloatConversion<From, To> : std::true_type
		{};
	} // namespace _internal

	template <_internal::IntOrEnum To, _internal::IntOrEnum From>
	[[nodiscard]] constexpr To NumCast(From x) noexcept
		requires(_internal::IsSigned<To> == _internal::IsSigned<From> && sizeof(To) >= sizeof(From))
	{
		return static_cast<To>(x);
	}

	template <std::floating_point To, std::floating_point From>
	[[nodiscard]] constexpr To NumCast(From x) noexcept requires(sizeof(To) >= sizeof(From))
	{
		return static_cast<To>(x);
	}

	template <std::floating_point To, _internal::IntOrEnum From>
	[[nodiscard]] constexpr To NumCast(From x) noexcept requires(sizeof(To) > sizeof(From))
	{
		return static_cast<To>(x);
	}

	template <_internal::IntOrEnum To, _internal::IntOrEnum From>
	[[nodiscard]] To NumCast(From x)
		requires(_internal::IsSigned<To> != _internal::IsSigned<From> || sizeof(To) < sizeof(From))
	{
		const To res = static_cast<To>(x);
		static constexpr bool fromSigned = _internal::IsSigned<From>;
		static constexpr bool toSigned = _internal::IsSigned<To>;

		if constexpr (fromSigned != toSigned)
		{
			APOLLO_ASSERT(
				!(x & _internal::SignBit<From>),
				"NumCast result {} differs from original value {}",
				_internal::ToUnderlying(res),
				_internal::ToUnderlying(x));
		}
		if constexpr ((sizeof(To) < sizeof(From)))
		{
			APOLLO_ASSERT(
				static_cast<From>(res) == x,
				"NumCast result {} differs from original value {}",
				_internal::ToUnderlying(res),
				_internal::ToUnderlying(x));
		}

		return res;
	}

	template <class To, class From>
	[[nodiscard]] To NumCast(From x) requires(_internal::LossyFloatConversion<From, To>::value)
	{
		const To res = static_cast<To>(x);
		APOLLO_ASSERT(
			static_cast<From>(res) == x,
			"Detected lossy floating point conversion from {} to {}",
			_internal::ToUnderlying(x),
			_internal::ToUnderlying(res));
		return res;
	}
} // namespace apollo