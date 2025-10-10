#include <catch2/catch_test_macros.hpp>
#include <core/Json.hpp>

#define JSON_TEST(name) TEST_CASE(name, "[json]")
namespace brk::json::ut {
	struct S1
	{
		int val1 = 0;
		std::string_view val2;

		static constexpr FieldList<&S1::val1, &S1::val2> JsonFields{ {
			{ "val1", true },
			{ "val2" },
		} };
	};

	static_assert(HasJsonFieldList<S1>);
	static_assert(!HasJsonFieldList<int>);
	static_assert(!HasJsonFieldList<void>);

	JSON_TEST("Load S1")
	{
		using ConverterT = Converter<S1>;
		const nlohmann::json j{
			{ "val1", 1 },
			{ "val2", "hello" },
		};
		S1 s;
		CHECK(ConverterT::FromJson(s, j));

		CHECK(s.val1 == 1);
		CHECK((s.val2 == "hello"));
	}

	JSON_TEST("Load S1 with default val1")
	{
		using ConverterT = Converter<S1>;
		const nlohmann::json j{
			{ "val2", "hello" },
		};
		S1 s;
		CHECK(ConverterT::FromJson(s, j));

		CHECK(s.val1 == 0);
		CHECK((s.val2 == "hello"));
	}

	JSON_TEST("Visit with wrong output type")
	{
		{
			const nlohmann::json j{
				{ "str", "hello" },
				{ "int", 1u },
			};
			int32 val = 0;
			CHECK_FALSE(Visit(val, j, "k"));
			CHECK(val == 0);

			CHECK(Visit(val, j, "int"));
			CHECK(val == 1);
		}
		{
			const nlohmann::json j{
				{"uint", 1},
			};
			uint32 x = 0;
			CHECK(Visit(x, j, "uint"));
			CHECK(x == 1);
		}
	}
} // namespace brk::json::ut

#undef JSON_TEST