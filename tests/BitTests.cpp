#include <catch2/catch_test_macros.hpp>
#include <core/Bit.hpp>
#include <format>

#define BIT_TEST(name) TEST_CASE(name, "[bits]")

namespace apollo::bitspan_ut {
	BIT_TEST("Const Iterator dereference")
	{
		static constexpr uint8 b[1] = { 1 };
		{
			constexpr BitIterator it{ b, 0 };
			static_assert(*it);
		}
		{
			constexpr BitIterator it{ b, 1 };
			static_assert(!*it);
		}
	}

	BIT_TEST("Iterator increment")
	{
		SECTION("Pre-increment")
		{
			static constexpr uint8 b = 1;
			BitIterator it{ &b, 0 };
			REQUIRE(*it == true);
			CHECK(*++it == false);
		}
		SECTION("Post-increment")
		{
			static constexpr uint8 b = 1;
			BitIterator it{ &b, 0 };
			CHECK(*it++ == true);
			CHECK(*it++ == false);
		}
	}

	BIT_TEST("Bit reference")
	{
		uint8 b = 0;
		BitIterator it{ &b, 0 };
		*it = true;
		CHECK(b == 1);
		CHECK(*it == true);
	}

	BIT_TEST("Bit iterator comparison")
	{
		static constexpr uint8 b[2] = {};
		constexpr BitIterator start{ b };
		constexpr BitIterator end{ b + 1 };
		static_assert(BitIterator{ b } == start);
		static_assert(start != end);
	}

	BIT_TEST("BitIterator::Set")
	{
		static constexpr bool b = []()
		{
			uint8 byte = 0;
			BitIterator<uint8> it{ &byte };
			it.Set();
			return byte & 1;
		}();
		static_assert(b);
	}
	BIT_TEST("BitIterator::Clear")
	{
		static constexpr bool b = []()
		{
			uint8 byte = 1;
			BitIterator<uint8> it{ &byte };
			it.Clear();
			return byte & 1;
		}();
		static_assert(!b);
	}

	BIT_TEST("Size of empty bitspan")
	{
		constexpr BitSpan<const uint8> span;
		static_assert(span.GetSize() == 0);
	}
	BIT_TEST("Size of non-empty bitspan")
	{
		static constexpr uint8 arr[] = { 1, 2 };
		{
			constexpr BitSpan span{ arr, 16 };
			static_assert(span.GetSize() == 16);
		}
		{
			constexpr BitSpan span{ arr, 15 };
			static_assert(span.GetSize() == 15);
		}
	}

	BIT_TEST("Span iteration")
	{
		static constexpr uint8 b = 0b10101010;
		size_t i = 0;
		const BitSpan span{ &b, 8 };

		for (const bool x : span)
		{
			if (x != bool(i++ % 2))
			{
				FAIL_CHECK(std::format("Error at index {}: expected {}, got {}", i, !x, x));
				break;
			}
		}
	}

	BIT_TEST("Const [] operator")
	{
		static constexpr uint8 b = 5;
		constexpr BitSpan span{ &b, 8 };
		static_assert(span[0]);
		static_assert(!span[1]);
		static_assert(span[2]);
	}

	BIT_TEST("Non-const [] operator")
	{
		uint8 b = 1 << 7;
		const BitSpan span{ &b, 8 };

		REQUIRE(!span[0]);
		span[0] = true;
		CHECK(span[0]);

		REQUIRE(span[7]);
		span[7] = false;
		CHECK(!span[7]);
	}
} // namespace apollo::bitspan_ut