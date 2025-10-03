#pragma once

namespace brk {
	template <class T, class U>
	[[nodiscard]] constexpr auto Min(T&& a, U&& b) noexcept(noexcept(a < b))
	{
		if (a < b)
			return a;
		return b;
	}

	template <class T, class U>
	[[nodiscard]] constexpr auto Max(T&& a, U&& b) noexcept(noexcept(a < b))
	{
		if (a > b)
			return a;
		return b;
	}
} // namespace brk