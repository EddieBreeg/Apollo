#include <catch2/catch_test_macros.hpp>
#include <core/NumConv.hpp>

#define NUMCONV_TEST(name) TEST_CASE(name, "[num_conv]")

namespace apollo::numconv_ut {
	enum class E1 : int32
	{
		Min = INT32_MIN,
		Val1 = 1,
		Max = INT32_MAX,
	};
	enum class E2 : uint32
	{
		Val1 = 1,
		Max = UINT32_MAX
	};
	enum class E3 : int64
	{
		Val1 = 1,
		Max = INT64_MAX
	};
	enum class E4 : uint64
	{
		Val1 = 1,
		Max = UINT64_MAX
	};

	static_assert(_internal::LossyFloatConversion<double, float>::value);
	static_assert(_internal::LossyFloatConversion<int32, float>::value);
	static_assert(_internal::LossyFloatConversion<uint32, float>::value);
	static_assert(_internal::LossyFloatConversion<int64, float>::value);
	static_assert(_internal::LossyFloatConversion<uint64, float>::value);
	static_assert(_internal::LossyFloatConversion<int64, double>::value);
	static_assert(_internal::LossyFloatConversion<uint64, double>::value);
	static_assert(_internal::LossyFloatConversion<float, int32>::value);
	static_assert(_internal::LossyFloatConversion<float, uint32>::value);
	static_assert(!_internal::LossyFloatConversion<float, double>::value);
	static_assert(!_internal::LossyFloatConversion<int32, double>::value);
	static_assert(!_internal::LossyFloatConversion<uint32, double>::value);
	static_assert(std::is_same_v<_internal::UnderlyingT<E1>::Type, int32>);
	static_assert(_internal::IsSigned<E1>);

	NUMCONV_TEST("T to T")
	{
		static_assert(NumCast<int32>(1) == 1);
		static_assert(NumCast<uint32>(1u) == 1u);
		static_assert(NumCast<float>(1.0f) == 1.0f);
		static_assert(NumCast<double>(1.0) == 1.0);
		static_assert(NumCast<E1>(E1::Val1) == E1::Val1);
	}

	NUMCONV_TEST("Small Number to Big Number")
	{
		static_assert(NumCast<int64>(1) == 1ll);
		static_assert(NumCast<uint64>(1u) == 1ull);
		static_assert(NumCast<double>(1.0f) == 1.0);
		static_assert(NumCast<double>(1) == 1.0);
		static_assert(NumCast<double>(1u) == 1.0);
		static_assert(NumCast<E1>(1) == E1::Val1);
		static_assert(NumCast<double>(E1::Val1) == 1.0);
	}

	NUMCONV_TEST("Big Number to Small Number")
	{
		CHECK(NumCast<int32>(1ll) == 1);
		CHECK(NumCast<uint32>(1ull) == 1u);
		CHECK(NumCast<int32>(int64(INT32_MIN)) == INT32_MIN);
		CHECK(NumCast<E1>(1ll) == E1::Val1);
		CHECK(NumCast<E1>(1ull) == E1::Val1);
		CHECK(NumCast<E1>(int64(INT32_MIN)) == E1::Min);
		CHECK_THROWS(NumCast<uint32>(1ull << 32));
		CHECK_THROWS(NumCast<E1>(1ull << 32));
	}

	NUMCONV_TEST("Signed to Unsigned")
	{
		CHECK(NumCast<uint32>(INT32_MAX) == uint32(INT32_MAX));
		CHECK(NumCast<E2>(1) == E2::Val1);
		CHECK_THROWS(NumCast<uint32>(-1));
		CHECK_THROWS(NumCast<E2>(-1));
	}

	NUMCONV_TEST("Unsigned to Signed")
	{
		CHECK(NumCast<int32>(1u) == 1);
		CHECK(NumCast<E1>(1u) == E1::Val1);
		CHECK_THROWS(NumCast<int32>(UINT32_MAX));
		CHECK_THROWS(NumCast<E1>(UINT32_MAX));
	}

	NUMCONV_TEST("double to float")
	{
		CHECK(NumCast<float>(1.0) == 1.0f);
		CHECK_THROWS(NumCast<float>(std::numeric_limits<double>::denorm_min()));
		CHECK_THROWS(NumCast<float>(double(UINT32_MAX)));
	}

	NUMCONV_TEST("float to integer")
	{
		CHECK(NumCast<int32>(1.0f) == 1);
		CHECK(NumCast<int32>(1.0) == 1);
		CHECK(NumCast<uint32>(1.0f) == 1u);
		CHECK(NumCast<uint32>(1.0) == 1u);
		CHECK(NumCast<E1>(1.0f) == E1::Val1);
		CHECK(NumCast<E1>(1.0) == E1::Val1);
		CHECK(NumCast<E2>(1.0f) == E2::Val1);
		CHECK(NumCast<E2>(1.0) == E2::Val1);

		CHECK_THROWS(NumCast<int32>(1.5f));
		CHECK_THROWS(NumCast<int32>(1.5));
		CHECK_THROWS(NumCast<uint32>(1.5f));
		CHECK_THROWS(NumCast<uint32>(1.5));
		CHECK_THROWS(NumCast<int64>(1.5f));
		CHECK_THROWS(NumCast<int64>(1.5));
		CHECK_THROWS(NumCast<uint64>(1.5f));
		CHECK_THROWS(NumCast<uint64>(1.5));
		CHECK_THROWS(NumCast<E1>(1.5f));
		CHECK_THROWS(NumCast<E2>(1.5f));
	}

	NUMCONV_TEST("integer to float")
	{
		CHECK(NumCast<float>(1) == 1.0f);
		CHECK(NumCast<float>(1u) == 1.0f);
		CHECK(NumCast<float>(1ll) == 1.0f);
		CHECK(NumCast<float>(1ull) == 1.0f);
		CHECK(NumCast<float>(-1ll) == -1.0f);
		CHECK(NumCast<float>(E1::Val1) == 1.0f);
		
		CHECK(NumCast<double>(1) == 1.0);
		CHECK(NumCast<double>(1u) == 1.0);
		CHECK(NumCast<double>(1ll) == 1.0);
		CHECK(NumCast<double>(1ull) == 1.0);
		CHECK(NumCast<double>(-1ll) == -1.0);
		CHECK(NumCast<double>(INT32_MAX) == double(INT32_MAX));
		CHECK(NumCast<double>(UINT32_MAX) == double(UINT32_MAX));
		CHECK(NumCast<double>(E3::Val1) == 1.0);
		CHECK(NumCast<double>(E4::Val1) == 1.0);

		CHECK_THROWS(NumCast<float>(INT32_MAX));
		CHECK_THROWS(NumCast<float>(UINT32_MAX));
		CHECK_THROWS(NumCast<float>(E1::Max));
		CHECK_THROWS(NumCast<float>(E2::Max));
		CHECK_THROWS(NumCast<double>(INT64_MAX));
		CHECK_THROWS(NumCast<double>(UINT64_MAX));
		CHECK_THROWS(NumCast<double>(E3::Max));
		CHECK_THROWS(NumCast<double>(E4::Max));
	}
} // namespace apollo::numconv_ut

#undef NUMCONV_TEST