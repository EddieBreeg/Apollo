#pragma once

#include "Util.hpp"

namespace brk {
	template <class T>
	struct Rectangle
	{
		T x, y, width, height;

		constexpr Rectangle& operator+=(const Rectangle& other) noexcept
		{
			const T x1 = Min(x, other.x);
			const T y1 = Min(y, other.y);
			const T x2 = Max(x + width, other.x + other.width);
			const T y2 = Max(y + height, other.y + other.height);

			x = x1;
			y = y1;
			width = x2 - x1;
			height = y2 - y1;
			return *this;
		}

		template <class U>
		[[nodiscard]] constexpr bool operator==(const Rectangle<U>& other) const noexcept
		{
			return (x == other.x) && (y == other.y) && (width == other.width) &&
				   (height == other.height);
		}

		template <class U>
		[[nodiscard]] constexpr bool operator!=(const Rectangle<U>& other) const noexcept
		{
			return (x != other.x) || (y != other.y) || (width != other.width) ||
				   (height != other.height);
		}
	};

	template <class T>
	[[nodiscard]] constexpr Rectangle<T> operator+(Rectangle<T> a, const Rectangle<T>& b) noexcept
	{
		return a += b;
	}
}; // namespace brk