#include <catch2/catch_test_macros.hpp>
#include <core/Enum.hpp>

namespace {
	enum class E : int32
	{
		Val0 = 0,
		Val1 = 1,
		Val2 = 2,
		Val3 = 3,
	};

	using namespace apollo::enum_operators;

	static_assert(E::Val0 < E::Val1);
	static_assert(E::Val0 <= E::Val1);
	static_assert(E::Val1 > E::Val0);
	static_assert(E::Val1 >= E::Val0);

	static_assert((E::Val0 ^ E::Val1) == E::Val1);
	static_assert((E::Val3 ^ E::Val1) == E::Val2);
	static_assert((E::Val1 | E::Val2) == E::Val3);
	static_assert(!E::Val0);
	static_assert((E::Val3 & E::Val1) == E::Val1);
	static_assert(~E::Val1 == E(0xfffffffe));
	static_assert((E::Val3 & ~E::Val1) == E::Val2);

	TEST_CASE("Enum operator ^=", "[enums]")
	{
		E val = E::Val1;
		val ^= E::Val2;
		CHECK(val == E::Val3);
	}
	TEST_CASE("Enum operator &=", "[enums]")
	{
		E val = E::Val1;
		val &= E::Val2;
		CHECK(val == E::Val0);
	}
	TEST_CASE("Enum operator |=", "[enums]")
	{
		E val = E::Val1;
		val |= E::Val2;
		CHECK(val == E::Val3);
	}
} // namespace
