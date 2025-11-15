#include <catch2/catch_test_macros.hpp>
#include <core/UniqueFunction.hpp>

#define UF_TEST(name) TEST_CASE(name, "[poly][unique_function]")

namespace apollo::poly::ut {
	struct F1
	{
		static inline int32 s_DestructionCount = 0;
		void operator()() const noexcept {};

		F1() = default;

		F1(const F1&) = delete;
		F1 operator=(const F1&) = delete;

		F1(F1&&) = default;
		F1& operator=(F1&&) = default;

		~F1() { ++s_DestructionCount; }
	};

	struct F2
	{
		std::byte m_Buf[32]; // too big for small object optimization
		void operator()() const noexcept {}

		F2() = default;

		F2(const F2&) = delete;
		F2 operator=(const F2&) = delete;

		F2(F2&&) = default;
		F2& operator=(F2&&) = default;

		void* operator new(size_t n)
		{
			++s_AllocCount;
			return ::operator new(n);
		}

		void operator delete(void* ptr)
		{
			--s_AllocCount;
			return ::operator delete(ptr);
		}

		~F2() { ++s_DestructionCount; }

		static inline int32 s_DestructionCount = 0;
		static inline int32 s_AllocCount = 0;
	};

	template <bool Small = true>
	struct F3
	{
		int32 m_Value = 0;
		F3(int32 val = 0)
			: m_Value(val)
		{}
		int operator()() const noexcept { return m_Value; }

		F3(const F3&) = delete;
		F3 operator=(const F3&) = delete;

		F3(F3&&) = default;
		F3& operator=(F3&&) = default;
	};

	template <>
	struct F3<false>
	{
		int32 m_Value = 0;
		std::byte m_Buf[28];
		F3(int32 val = 0)
			: m_Value(val)
		{}

		int operator()() const noexcept { return m_Value; }
		F3(const F3&) = delete;
		F3 operator=(const F3&) = delete;

		F3(F3&&) = default;
		F3& operator=(F3&&) = default;
	};

	UF_TEST("Empty function")
	{
		const UniqueFunction<void()> f;
		CHECK_FALSE(f);
	}

	UF_TEST("Destruction test")
	{
		{
			const UniqueFunction<void()> f{ F1{} };
			CHECK(f);
			F1::s_DestructionCount = 0;
		}
		CHECK(F1::s_DestructionCount == 1);

		F2::s_AllocCount = 0;
		{
			const UniqueFunction<void()> f{ F2{} };
			CHECK(f);
			CHECK(F2::s_AllocCount == 1);
			F2::s_DestructionCount = 0;
		}
		CHECK(F2::s_DestructionCount == 1);
		CHECK(F2::s_AllocCount == 0);
	}

	UF_TEST("Swap test")
	{
		UniqueFunction<void()> f1{ F1{} };
		REQUIRE(f1);
		F1::s_DestructionCount = 0;
		{
			UniqueFunction<void()> f2;
			REQUIRE_FALSE(f2);
			f2.Swap(f1);
			CHECK(f2);
			CHECK_FALSE(f1);
		}
		CHECK(F1::s_DestructionCount == 1);
	}

	UF_TEST("Call Test")
	{
		{
			UniqueFunction<int32()> f{ F3<true>{ 1 } };
			CHECK(f() == 1);
		}
		{
			UniqueFunction<int32()> f{ F3<false>{ 1 } };
			CHECK(f() == 1);
		}
	}

	UF_TEST("Move Tests")
	{
		SECTION("Move construction")
		{
			{
				UniqueFunction<int32()> f1{ F3<true>{ 4 } };
				REQUIRE(f1);
				REQUIRE(f1() == 4);
				UniqueFunction<int32()> f2{ std::move(f1) };
				CHECK_FALSE(f1);
				CHECK(f2);
				CHECK(f2() == 4);
			}
			{
				UniqueFunction<int32()> f1{ F3<false>{ 4 } };
				REQUIRE(f1);
				REQUIRE(f1() == 4);
				UniqueFunction<int32()> f2{ std::move(f1) };
				CHECK_FALSE(f1);
				CHECK(f2);
				CHECK(f2() == 4);
			}
		}
		SECTION("Move assignment")
		{
			{
				UniqueFunction<int32()> f1{ F3<false>{ 1 } };
				REQUIRE(f1);
				REQUIRE(f1() == 1);

				UniqueFunction<int32()> f2{ F3<true>{ 2 } };
				REQUIRE(f2);
				REQUIRE(f2() == 2);

				f1 = std::move(f2);

				CHECK(f1);
				CHECK(f1() == 2);

				CHECK(f2);
				CHECK(f2() == 1);
			}
		}
	}

	UF_TEST("Assignment test")
	{
		{
			F1::s_DestructionCount = 0;
			UniqueFunction<void()> f1;
			f1 = F1{};
			REQUIRE(F1::s_DestructionCount == 1);
			CHECK(f1);
		}
		CHECK(F1::s_DestructionCount == 2);
		{
			UniqueFunction<void()> f1{ F1{} };
			REQUIRE(f1);
			F1::s_DestructionCount = 0;
			f1 = F1{};
			REQUIRE(F1::s_DestructionCount == 2);
			CHECK(f1);
		}
	}
} // namespace apollo::poly::ut