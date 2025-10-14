#include <catch2/catch_test_macros.hpp>
#include <core/RetainPtr.hpp>

#define RETAIN_PTR_TEST(name) TEST_CASE(name, "[retain_ptr]")

namespace apollo::retain_ptr::ut {
	struct S1
	{
		uint32 m_RefCount = 0;
		bool m_Deleted = false;

		void operator delete(void* ptr) { static_cast<S1*>(ptr)->m_Deleted = true; }

		struct RetainTraits
		{};
	};

	RETAIN_PTR_TEST("GetCount")
	{
		{
			const S1 s;
			CHECK(DefaultRetainTraits<S1>::GetCount(&s) == 0);
		}
		{
			const S1 s{ 1 };
			CHECK(DefaultRetainTraits<S1>::GetCount(&s) == 1);
		}
	}

	RETAIN_PTR_TEST("Increment")
	{
		S1 s;
		DefaultRetainTraits<S1>::Increment(&s);
		CHECK(s.m_RefCount == 1);
	}

	RETAIN_PTR_TEST("Decrement")
	{
		S1 s{ 2 };
		DefaultRetainTraits<S1>::Decrement(&s);
		CHECK(s.m_RefCount == 1);
	}

	RETAIN_PTR_TEST("Decrement and Delete")
	{
		S1 s{ 1 };
		DefaultRetainTraits<S1>::Decrement(&s);
		CHECK(s.m_RefCount == 0);
		CHECK(s.m_Deleted);
	}

	RETAIN_PTR_TEST("Empty RetainPtr")
	{
		RetainPtr<S1> ptr;
		CHECK(!ptr);
	}

	RETAIN_PTR_TEST("Adopt")
	{
		S1 s;
		RetainPtr<S1> ptr{ &s, Adopt_t{} };
		REQUIRE(!s.m_Deleted);
		CHECK(ptr.GetCount() == 0);
	}

	RETAIN_PTR_TEST("Retain")
	{
		S1 s;
		RetainPtr<S1> ptr{ &s, Retain_t{} };
		REQUIRE(!s.m_Deleted);
		CHECK(ptr.GetCount() == 1);
	}

	RETAIN_PTR_TEST("Default Action")
	{
		S1 s;
		RetainPtr<S1> ptr{ &s };
		REQUIRE(!s.m_Deleted);
		CHECK(ptr.GetCount() == 1);
	}

	RETAIN_PTR_TEST("Destroy")
	{
		S1 s;
		{
			RetainPtr<S1> ptr{ &s };
		}
		CHECK(s.m_RefCount == 0);
		CHECK(s.m_Deleted);
	}

	RETAIN_PTR_TEST("Reset Adopt")
	{
		SECTION("nullptr")
		{
			S1 s1;
			RetainPtr<S1> ptr{ &s1 };
			REQUIRE(ptr.GetCount() == 1);
			ptr.Reset(nullptr, Adopt_t{});

			CHECK(s1.m_RefCount == 0);
			CHECK(s1.m_Deleted);
		}
		SECTION("!nullptr")
		{
			S1 s1;
			RetainPtr<S1> ptr{ &s1 };
			REQUIRE(ptr.GetCount() == 1);
			S1 s2;
			ptr.Reset(&s2, Adopt_t{});

			CHECK(s1.m_RefCount == 0);
			CHECK(s1.m_Deleted);
			CHECK(s2.m_RefCount == 0);
			CHECK_FALSE(s2.m_Deleted);
		}
	}

	RETAIN_PTR_TEST("Reset Retain")
	{
		SECTION("nullptr")
		{
			S1 s1;
			RetainPtr<S1> ptr{ &s1 };
			REQUIRE(ptr.GetCount() == 1);
			ptr.Reset(nullptr, Retain_t{});

			CHECK(s1.m_RefCount == 0);
			CHECK(s1.m_Deleted);
		}
		SECTION("!nullptr")
		{
			S1 s1;
			RetainPtr<S1> ptr{ &s1 };
			REQUIRE(ptr.GetCount() == 1);
			S1 s2;
			ptr.Reset(&s2, Retain_t{});

			CHECK(s1.m_RefCount == 0);
			CHECK(s1.m_Deleted);
			CHECK(s2.m_RefCount == 1);
			CHECK_FALSE(s2.m_Deleted);
		}
	}

	RETAIN_PTR_TEST("Reset Default")
	{
		SECTION("nullptr")
		{
			S1 s1;
			RetainPtr<S1> ptr{ &s1 };
			REQUIRE(ptr.GetCount() == 1);
			ptr.Reset();

			CHECK(s1.m_RefCount == 0);
			CHECK(s1.m_Deleted);
		}
		SECTION("!nullptr")
		{
			S1 s1;
			RetainPtr<S1> ptr{ &s1 };
			REQUIRE(ptr.GetCount() == 1);
			S1 s2;
			ptr.Reset(&s2);

			CHECK(s1.m_RefCount == 0);
			CHECK(s1.m_Deleted);
			CHECK(s2.m_RefCount == 1);
			CHECK_FALSE(s2.m_Deleted);
		}
	}

	RETAIN_PTR_TEST("Copy")
	{
		SECTION("Copy Construction")
		{
			S1 s;
			const RetainPtr ptr{ &s };
			REQUIRE(s.m_RefCount == 1);
			{
				const RetainPtr ptr2{ ptr };
				CHECK(ptr == ptr2);
				CHECK(ptr.GetCount() == 2);
				CHECK(ptr2.GetCount() == 2);
			}
			CHECK(s.m_RefCount == 1);
			CHECK_FALSE(s.m_Deleted);
		}
		SECTION("Copy Assignment")
		{
			S1 s1;
			S1 s2;
			RetainPtr ptr{ &s1 };

			REQUIRE(s1.m_RefCount == 1);
			{
				const RetainPtr ptr2{ &s2 };
				REQUIRE(s2.m_RefCount == 1);
				ptr = ptr2;

				CHECK(s1.m_RefCount == 0);
				CHECK(s1.m_Deleted);

				CHECK(ptr == ptr2);
				CHECK(s2.m_RefCount == 2);
			}
			CHECK(s2.m_RefCount == 1);
			CHECK_FALSE(s2.m_Deleted);
		}
	}

	RETAIN_PTR_TEST("Move")
	{
		SECTION("Move Construction")
		{
			S1 s;
			RetainPtr ptr{ &s };
			REQUIRE(s.m_RefCount == 1);
			{
				const RetainPtr ptr2{ std::move(ptr) };
				CHECK_FALSE(ptr);
				CHECK(s.m_RefCount == 1);
				CHECK_FALSE(s.m_Deleted);
			}
			CHECK(s.m_RefCount == 0);
			CHECK(s.m_Deleted);
		}
		SECTION("Move Assignment")
		{
			S1 s1, s2;
			RetainPtr ptr{ &s1 };
			REQUIRE(s1.m_RefCount == 1);
			{
				RetainPtr ptr2{ &s2 };
				REQUIRE(s2.m_RefCount == 1);

				ptr = std::move(ptr2);
				CHECK(ptr.Get() == &s2);
				CHECK(ptr2.Get() == &s1);

				CHECK(s1.m_RefCount == 1);
				CHECK(s2.m_RefCount == 1);

				CHECK_FALSE(s1.m_Deleted);
				CHECK_FALSE(s2.m_Deleted);
			}
			CHECK(s1.m_RefCount == 0);
			CHECK(s1.m_Deleted);
		}
	}

	RETAIN_PTR_TEST("Release")
	{
		S1 s;
		RetainPtr ptr{ &s };
		CHECK(ptr.Release() == &s);
		CHECK(s.m_RefCount == 1);
		CHECK_FALSE(s.m_Deleted);
		CHECK_FALSE(ptr);
	}

	RETAIN_PTR_TEST("Swap")
	{
		S1 s, s2;
		RetainPtr ptr{ &s };
		RetainPtr ptr2{ &s2 };
		ptr.Swap(ptr2);

		CHECK(s.m_RefCount == 1);
		CHECK(s2.m_RefCount == 1);
		CHECK_FALSE(s.m_Deleted);
		CHECK_FALSE(s2.m_Deleted);
		CHECK(ptr.Get() == &s2);
		CHECK(ptr2.Get() == &s);
	}

	RETAIN_PTR_TEST("Dereference")
	{
		S1 s;
		RetainPtr<S1> ptr{ &s };
		CHECK(ptr->m_RefCount == 1);
		CHECK_FALSE(ptr->m_Deleted);

		CHECK((*ptr).m_RefCount == 1);
		CHECK_FALSE((*ptr).m_Deleted);
	}
} // namespace apollo::retain_ptr::ut

#undef RETAIN_PTR_TEST