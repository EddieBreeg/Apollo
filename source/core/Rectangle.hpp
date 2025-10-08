#pragma once

#include "Util.hpp"

namespace brk {
	template <class T>
	struct Rectangle
	{
		T x0, y0, x1, y1;

		constexpr Rectangle& operator+=(const Rectangle& other) noexcept
		{
			x0 = Min(x0, other.x0);
			y0 = Min(y0, other.y0);
			x1 = Max(x1, other.x1);
			y1 = Max(y1, other.y1);
			return *this;
		}

		template <class U>
		[[nodiscard]] constexpr bool operator==(const Rectangle<U>& other) const noexcept
		{
			return (x0 == other.x0) && (y0 == other.y0) && (x1 == other.x1) && (y1 == other.y1);
		}

		template <class U>
		[[nodiscard]] constexpr bool operator!=(const Rectangle<U>& other) const noexcept
		{
			return (x0 != other.x0) || (y0 != other.y0) || (x1 != other.x1) || (y1 != other.y1);
		}

		[[nodiscard]] constexpr T GetWidth() const noexcept { return x1 - x0; }
		[[nodiscard]] constexpr T GetHeight() const noexcept { return y1 - y0; }
	};

	template <class T>
	[[nodiscard]] constexpr Rectangle<T> operator+(Rectangle<T> a, const Rectangle<T>& b) noexcept
	{
		return a += b;
	}
}; // namespace brk