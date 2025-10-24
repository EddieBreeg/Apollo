#include <catch2/catch_test_macros.hpp>
#include <core/Util.hpp>

#define UTIL_TEST(name) TEST_CASE(name, "[util]")

namespace apollo::util_ut {
	struct S1
	{
		int32_t m_Val;
	};

	struct S2
	{
		int32_t m_Val;
		S2& operator=(S2&&) = delete;

		void Swap(S2& other) noexcept
		{
			apollo::Swap(m_Val, other.m_Val);
		}
	};

	struct S3
	{
		int32_t m_Val;
		S3& operator=(S3&&) = delete;

		void swap(S3& other) noexcept
		{
			apollo::Swap(m_Val, other.m_Val);
		}
	};

	UTIL_TEST("Swap of trivial objects")
	{
		SECTION("Integer Swap")
		{
			int32_t x = 0, y = 1;
			Swap(x, y);
			CHECK(x == 1);
			CHECK(y == 0);
		}
		SECTION("S1 Swap")
		{
			S1 a{ 1 }, b{ 2 };
			Swap(a, b);
			CHECK(a.m_Val == 2);
			CHECK(b.m_Val == 1);
		}
	}
	
	UTIL_TEST("Swap method")
	{
		S2 a{ 1 }, b{ 2 };
		Swap(a, b);
		CHECK(a.m_Val == 2);
		CHECK(b.m_Val == 1);
	}

	UTIL_TEST("swap method (STL)")
	{
		S3 a{ 1 }, b{ 2 };
		Swap(a, b);
		CHECK(a.m_Val == 2);
		CHECK(b.m_Val == 1);
	}

	UTIL_TEST("Array Swap")
	{
		SECTION("Swap arrays of ints")
		{
			int32_t a[] = {1, 2};
			int32_t b[] = {3, 4};
			Swap(a, b);
			CHECK(a[0] == 3);
			CHECK(a[1] == 4);
			CHECK(b[0] == 1);
			CHECK(b[1] == 2);
		}
	}
} // namespace apollo::util_ut