#include <PCH.hpp>
#include <catch2/catch_test_macros.hpp>

#define RECTANGLE_TEST(name) TEST_CASE(name, "[util][rectangle]")

namespace brk::util_ut {
	RECTANGLE_TEST("+ operator")
	{
		constexpr RectI32 r1{
			-1,
			0,
			1,
			2,
		};
		constexpr RectI32 r2{
			0,
			-1,
			2,
			1,
		};
		static_assert((r1 + r2) == RectI32{ -1, -1, 2, 2 });
		static_assert(r1.GetWidth() == 2);
		static_assert(r1.GetHeight() == 2);

		static_assert(r2.GetWidth() == 2);
		static_assert(r2.GetHeight() == 2);

		static_assert((r1 + r2).GetWidth() == 3);
		static_assert((r1 + r2).GetHeight() == 3);
	}
} // namespace brk::util_ut
