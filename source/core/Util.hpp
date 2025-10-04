#pragma once

namespace brk {
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
} // namespace brk