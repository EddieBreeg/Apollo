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
			return static_cast<E>(ToUnderlying(a) | ToUnderlying(b));
		}

		template <class E>
		[[nodiscard]] constexpr E operator&(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return static_cast<E>(ToUnderlying(a) & ToUnderlying(b));
		}

		template <class E>
		[[nodiscard]] constexpr E operator^(E a, E b) noexcept requires std::is_enum_v<E>
		{
			return static_cast<E>(ToUnderlying(a) ^ ToUnderlying(b));
		}

		template <class E>
		[[nodiscard]] constexpr E operator~(E x) noexcept requires std::is_enum_v<E>
		{
			return static_cast<E>(~ToUnderlying(x));
		}

		template <class E>
		[[nodiscard]] constexpr bool operator!(E x) noexcept requires std::is_enum_v<E>
		{
			return !ToUnderlying(x);
		}

		template <class E>
		constexpr E& operator^=(E& a, E b) noexcept requires std::is_enum_v<E>
		{
			return a = a ^ b;
		}

		template <class E>
		constexpr E& operator|=(E& a, E b) noexcept requires std::is_enum_v<E>
		{
			return a = a | b;
		}

		template <class E>
		constexpr E& operator&=(E& a, E b) noexcept requires std::is_enum_v<E>
		{
			return a = a & b;
		}
	} // namespace enum_operators

	using namespace enum_operators;

	template <class E>
	requires(std::is_enum_v<E>) struct EnumFlags
	{
		E m_Value = static_cast<E>(0);

		constexpr EnumFlags() = default;
		constexpr EnumFlags(const EnumFlags&) noexcept = default;
		constexpr EnumFlags& operator=(const EnumFlags&) noexcept = default;
		constexpr EnumFlags& operator=(E flags) noexcept
		{
			m_Value = flags;
			return *this;
		}

		constexpr ~EnumFlags() = default;

		constexpr EnumFlags(E flags) noexcept
			: m_Value(flags)
		{}
		constexpr operator E() const noexcept { return m_Value; }

		[[nodiscard]] constexpr bool HasAny(E mask) noexcept { return bool(m_Value & mask); }

		[[nodiscard]] constexpr bool HasAll(E mask) noexcept { return (m_Value & mask) == mask; }

		constexpr void Set(E mask) noexcept { m_Value |= mask; }
		// If bit is true, equivalent to Set(mask), otherwise Clear(mask)
		constexpr void Set(E mask, bool bit) noexcept
		{
			m_Value = (m_Value & ~mask) | (bit * mask);
		}
		constexpr void Clear(E mask) noexcept { m_Value &= ~mask; }
		constexpr void Flip(E mask) noexcept { m_Value ^= mask; }
	};
} // namespace brk