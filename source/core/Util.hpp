#pragma once

#include <cstdint>

namespace apollo {
	[[nodiscard]] constexpr auto Min(auto&& a, auto&& b) noexcept(noexcept(a < b))
	{
		if (a < b)
			return a;
		return b;
	}

	[[nodiscard]] constexpr auto Max(auto&& a, auto&& b) noexcept(noexcept(a < b))
	{
		if (a > b)
			return a;
		return b;
	}

	template <class T, class U, class V>
	[[nodiscard]] constexpr auto Clamp(T&& v, U&& min, V&& max) noexcept(
		noexcept(v < min) && noexcept(v > max))
	{
		if (v < min)
			return min;
		if (v > max)
			return max;
		return v;
	}

	[[nodiscard]] constexpr bool IsPowerOfTwo(auto&& val) noexcept
	{
		return (val & (~val + 1)) == val;
	}

	[[nodiscard]] constexpr uint32_t Padding(uint32_t offset, uint32_t alignment) noexcept
	{
		const uint32_t mask = alignment - 1;
		return (alignment - (offset & mask)) & mask;
	}

	[[nodiscard]] constexpr uint32_t Align(uint32_t offset, uint32_t alignment) noexcept
	{
		return offset + Padding(offset, alignment);
	}
} // namespace apollo