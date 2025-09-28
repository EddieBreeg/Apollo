#include <catch2/catch_test_macros.hpp>
#include <core/Json.hpp>
#include <core/ULID.hpp>

#define ULID_TEST(name) TEST_CASE(name, "[ulid]")

namespace {
	constexpr std::string_view g_StrId1{ "01HWPC5Y8GZZ3A6AQVX4PYRXHC" };
	constexpr std::string_view g_InvalidStrId1{ "01HWPC5Y8GZZ3A6AQVX4PYRXH$" };
	constexpr std::string_view g_InvalidStrId2{ "01HWPC5Y8GZZ3A6AQVX4" };

	constexpr brk::ULID g_Id1{ 0x018f2cc2f910, 0xffc6, 0xa32afbe92dec762c };

	static_assert(brk::json::JsonEnabledType<brk::ULID>);
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
		constexpr ULID id = brk::ULID::FromString(g_StrId1);
		static_assert(id == g_Id1);
	}

	ULID_TEST("Conversion from malformed string")
	{
		constexpr ULID id = brk::ULID::FromString(g_InvalidStrId1);
		static_assert(!id);
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

	TEST_CASE("Conversion from json", "[json][ulid]")
	{
		const nlohmann::json j = g_StrId1;
		ULID id;
		CHECK(id.FromJson(j));
		CHECK((id == g_Id1));
	}

	TEST_CASE("Conversion from non-string json", "[json][ulid]")
	{
		{
			const nlohmann::json j; // null
			ULID id;
			CHECK_FALSE(id.FromJson(j));
			CHECK_FALSE(id);
		}
		{
			const nlohmann::json j = 1;
			ULID id;
			CHECK_FALSE(id.FromJson(j));
			CHECK_FALSE(id);
		}
	}
	TEST_CASE("Conversion from malformed json ULID string", "[json][ulid]")
	{
		const nlohmann::json j = g_InvalidStrId1;
		ULID id;
		CHECK(id.FromJson(j));
		CHECK_FALSE(id);
	}
	TEST_CASE("Conversion from string json with invalid length", "[json][ulid]")
	{
		const nlohmann::json j = g_InvalidStrId2;
		ULID id;
		CHECK_FALSE(id.FromJson(j));
		CHECK_FALSE(id);
	}
} // namespace brk::ulid::ut

#undef ULID_TEST