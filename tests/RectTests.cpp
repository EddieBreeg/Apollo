#include <PCH.hpp>
#include <catch2/catch_test_macros.hpp>

#define RECTANGLE_TEST(name) TEST_CASE(name, "[util][rectangle]")

namespace brk::util_ut {
	RECTANGLE_TEST("+ operator")
	{
		constexpr RectI32 r1{
			-1,
			0,
			2,
			2,
		};
		constexpr RectI32 r2{
			0,
			-1,
			2,
			2,
		};
		static_assert((r1 + r2) == RectI32{ -1, -1, 3, 3 });
	}
} // namespace brk::util_ut
