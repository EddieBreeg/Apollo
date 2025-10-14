#include <catch2/catch_test_macros.hpp>
#include <core/Json.hpp>
#include <glm/glm.hpp>

#define JSON_TEST(name) TEST_CASE(name, "[json]")
namespace apollo::json::ut {
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
				{ "uint", 1 },
			};
			uint32 x = 0;
			CHECK(Visit(x, j, "uint"));
			CHECK(x == 1);
		}
	}

	JSON_TEST("Convert to vec1")
	{
		{
			const nlohmann::json json{ { "x", 1 } };
			glm::vec1 v{};
			CHECK(Converter<glm::vec1>::FromJson(v, json));
			CHECK(v.x == 1);
		}
		{
			const nlohmann::json json = nullptr;
			glm::vec1 v{};
			CHECK_FALSE(Converter<glm::vec1>::FromJson(v, json));
			CHECK(v.x == 0);
		}
	}
	JSON_TEST("Convert to vec2")
	{
		{
			const nlohmann::json json{
				{ "x", 1 },
				{ "y", 2 },
			};
			glm::vec2 v{};
			CHECK(Converter<glm::vec2>::FromJson(v, json));
			CHECK(v.x == 1);
			CHECK(v.y == 2);
		}
		{
			const nlohmann::json json = nullptr;
			glm::vec2 v{};
			CHECK_FALSE(Converter<glm::vec2>::FromJson(v, json));
			CHECK(v.x == 0);
			CHECK(v.y == 0);
		}
	}
	JSON_TEST("Convert to vec3")
	{
		{
			const nlohmann::json json{
				{ "x", 1 },
				{ "y", 2 },
				{ "z", 3 },
			};
			glm::vec3 v{};
			CHECK(Converter<glm::vec3>::FromJson(v, json));
			CHECK(v.x == 1);
			CHECK(v.y == 2);
			CHECK(v.z == 3);
		}
		{
			const nlohmann::json json = nullptr;
			glm::vec3 v{};
			CHECK_FALSE(Converter<glm::vec3>::FromJson(v, json));
			CHECK(v.x == 0);
			CHECK(v.y == 0);
			CHECK(v.y == 0);
		}
	}
	JSON_TEST("Convert to vec4")
	{
		{
			const nlohmann::json json{
				{ "x", 1 },
				{ "y", 2 },
				{ "z", 3 },
				{ "w", 4 },
			};
			glm::vec4 v{};
			CHECK(Converter<glm::vec4>::FromJson(v, json));
			CHECK(v.x == 1);
			CHECK(v.y == 2);
			CHECK(v.z == 3);
			CHECK(v.w == 4);
		}
		{
			const nlohmann::json json = nullptr;
			glm::vec4 v{};
			CHECK_FALSE(Converter<glm::vec4>::FromJson(v, json));
			CHECK(v.x == 0);
			CHECK(v.y == 0);
			CHECK(v.y == 0);
			CHECK(v.w == 0);
		}
	}

	JSON_TEST("Visit glm::vec")
	{
		const nlohmann::json j{
			{
				"v",
				{
					{ "x", 1 },
					{ "y", 2 },
					{ "z", 3 },
					{ "w", 4 },
				},
			},
		};
		glm::vec1 v1{};
		glm::vec2 v2{};
		glm::vec3 v3{};
		glm::vec4 v4{};
		CHECK(Visit(v1, j, "v"));
		CHECK(Visit(v2, j, "v"));
		CHECK(Visit(v3, j, "v"));
		CHECK(Visit(v4, j, "v"));

		CHECK(v1.x == 1);
		CHECK(v2.x == 1);
		CHECK(v3.x == 1);
		CHECK(v4.x == 1);

		CHECK(v2.y == 2);
		CHECK(v3.y == 2);
		CHECK(v4.y == 2);

		CHECK(v3.z == 3);
		CHECK(v4.z == 3);

		CHECK(v4.w == 4);
	}

	JSON_TEST("Convert rectangle")
	{
		{
			const nlohmann::json j{
				{ "x0", 1 },
				{ "x1", 2 },
				{ "y0", 3 },
				{ "y1", 4 },
			};
			RectU32 r = { 0, 0, 0, 0 };
			CHECK(Converter<RectU32>::FromJson(r, j));
			CHECK(r.x0 == 1);
			CHECK(r.x1 == 2);
			CHECK(r.y0 == 3);
			CHECK(r.y1 == 4);
		}
		{
			const nlohmann::json j{
				{ "x0", 1 },
				{ "x1", 2 },
				{ "y0", 3 },
			};
			RectU32 r = { 0, 0, 0, 0 };
			CHECK_FALSE(Converter<RectU32>::FromJson(r, j));
		}
	}

	static_assert(std::is_convertible_v<glm::vec1, nlohmann::json>);
	static_assert(std::is_convertible_v<glm::vec2, nlohmann::json>);
	static_assert(std::is_convertible_v<glm::vec3, nlohmann::json>);
	static_assert(std::is_convertible_v<glm::vec4, nlohmann::json>);
	static_assert(std::is_convertible_v<apollo::Rectangle<float>, nlohmann::json>);
	static_assert(std::is_convertible_v<apollo::Rectangle<uint32>, nlohmann::json>);
} // namespace apollo::json::ut

#undef JSON_TEST