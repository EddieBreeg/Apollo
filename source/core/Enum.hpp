#pragma once

#include <PCH.hpp>

namespace brk {
	template <class E>
	[[nodiscard]] constexpr auto ToUnderlying(E val) noexcept requires std::is_enum_v<E>
	{
		return static_cast<std::underlying_type_t<E>>(val);
	}

	namespace enum_operators {
		template <class E>
		[[nodiscard]] constexpr bool operator<(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return ToUnderlying(a) < ToUnderlying(b);
		}

		template <class E>
		[[nodiscard]] constexpr bool operator>(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return ToUnderlying(a) > ToUnderlying(b);
		}

		template <class E>
		[[nodiscard]] constexpr bool operator<=(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return ToUnderlying(a) <= ToUnderlying(b);
		}

		template <class E>
		[[nodiscard]] constexpr bool operator>=(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return ToUnderlying(a) >= ToUnderlying(b);
		}

		template <class E>
		[[nodiscard]] constexpr E operator|(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return ToUnderlying(a) | ToUnderlying(b);
		}

		template <class E>
		[[nodiscard]] constexpr E operator&(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return ToUnderlying(a) & ToUnderlying(b);
		}

		template <class E>
		[[nodiscard]] constexpr E operator^(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return ToUnderlying(a) ^ ToUnderlying(b);
		}

		template <class E>
		[[nodiscard]] constexpr E operator~(E x) noexcept requires std::is_enum_v<E>
		{
			return ~ToUnderlying(x);
		}

		template <class E>
		[[nodiscard]] constexpr bool operator!(E x) noexcept requires std::is_enum_v<E>
		{
			return !ToUnderlying(x);
		}
	} // namespace enum_operators

	using namespace enum_operators;
} // namespace brk