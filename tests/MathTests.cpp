#include <PCH.hpp>
#include <catch2/catch_test_macros.hpp>

#define MATH_TEST(name) TEST_CASE(name, "[math]")

namespace apollo::math_ut {
	MATH_TEST("MapRange from 0-1 to 0-1")
	{
		static_assert(MapRange(0, 0, 1, 0, 1) == 0);
		static_assert(MapRange(0.5f, 0, 1, 0, 1) == 0.5f);
		static_assert(MapRange(1, 0, 1, 0, 1) == 1);
	}
	MATH_TEST("MapRange from 0-1 to 0-2")
	{
		static_assert(MapRange(0, 0, 1, 0, 2) == 0);
		static_assert(MapRange(0.5f, 0, 1, 0, 2) == 1.0f);
		static_assert(MapRange(1, 0, 1, 0, 2) == 2.0f);
	}
	MATH_TEST("MapRange from 0-1 to 1-2")
	{
		static_assert(MapRange(0, 0, 1, 1, 2) == 1);
		static_assert(MapRange(0.5f, 0, 1, 1, 2) == 1.5f);
		static_assert(MapRange(1.0f, 0, 1, 1, 2) == 2.f);
	}
	MATH_TEST("MapRange from 1-3 to 0-1")
	{
		static_assert(MapRange(1, 1, 3, 0, 1) == 0);
		static_assert(MapRange(2, 1, 3, 0, 1) == 0.5f);
		static_assert(MapRange(3, 1, 3, 0, 1) == 1);
	}
} // namespace apollo::math_ut