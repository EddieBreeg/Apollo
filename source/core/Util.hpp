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
} // namespace brk