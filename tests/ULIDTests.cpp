#include <catch2/catch_test_macros.hpp>
#include <core/ULID.hpp>

#define ULID_TEST(name) TEST_CASE(name, "[ulid]")

namespace {
	constexpr std::string_view g_StrId1{ "01HWPC5Y8GZZ3A6AQVX4PYRXHC" };
	constexpr brk::ULID g_Id1{ 0x018f2cc2f910, 0xffc6, 0xa32afbe92dec762c };
} // namespace

namespace brk::ulid::ut {
	ULID_TEST("Null ULID")
	{
		constexpr ULID id{ 0, 0, 0 };
		static_assert(!id);
		static_assert(id == ULID{});
	}

	ULID_TEST("Conversion from string")
	{
		using namespace std::string_view_literals;
		constexpr ULID id = brk::ULID::FromString(g_StrId1);
		static_assert(id == g_Id1);
	}

	ULID_TEST("Conversion to string")
	{
		constexpr auto test = [](const ULID& id, const std::string_view expected)
		{
			char out[27] = {};
			id.ToChars(out);
			return out == expected;
		};
		static_assert(test(g_Id1, g_StrId1));
	}
} // namespace brk::ulid::ut

#undef ULID_TEST