#include <catch2/catch_test_macros.hpp>
#include <core/Coroutine.hpp>

#define CORO_TEST(name) TEST_CASE(name, "[coroutine]")

namespace {
	template <class T, bool EagerStart = false>
	struct P1 : public apollo::coro::NoopPromise<EagerStart>
	{
		T m_Res{};

		void return_value(T val) { m_Res = val; }
		std::suspend_always yield_value(T val)
		{
			m_Res = val;
			return {};
		}
	};

	apollo::coro::Coroutine<P1<int>> C1()
	{
		co_yield 1;
		co_return 2;
	}
	apollo::coro::Coroutine<P1<int>> C2(int val)
	{
		co_await std::suspend_always{};
		co_return val + 1;
	}
} // namespace

namespace apollo::coro::ut {
	CORO_TEST("Null coroutine to bool")
	{
		using C = Coroutine<>;
		static_assert(!C{});
	}

	CORO_TEST("No-op coroutine")
	{
		Coroutine<> noop{ std::noop_coroutine() };
		CHECK(noop);
		CHECK(!noop());
	}

	CORO_TEST("Yield 1, return 2")
	{
		auto coro = C1();
		REQUIRE(!coro.IsDone());
		coro();
		CHECK(coro->m_Res == 1);
		coro();
		CHECK(coro->m_Res == 2);
		CHECK(coro.IsDone());
	}

	CORO_TEST("Await then return 1")
	{
		auto coro = C2(0);
		REQUIRE(coro->m_Res == 0);
		coro();
		CHECK(coro->m_Res == 0);
		REQUIRE(coro());
		CHECK(coro->m_Res == 1);
	}

	CORO_TEST("Reset to null")
	{
		Coroutine<> coro = C1();
		REQUIRE(!coro.IsDone());
		coro.Reset();
		CHECK(!coro);
	}

	CORO_TEST("Reset to noop")
	{
		Coroutine<> coro = C1();
		REQUIRE(!coro.IsDone());
		const auto handle = std::noop_coroutine();
		coro.Reset(handle);
		CHECK(coro);
		CHECK((coro == handle));
	}

	CORO_TEST("Swap")
	{
		auto c1 = C1();
		c1();
		auto c2 = C1();
		REQUIRE(c1->m_Res == 1);
		REQUIRE(c2->m_Res == 0);

		c1.Swap(c2);
		REQUIRE(c1->m_Res == 0);
		REQUIRE(c2->m_Res == 1);
	}

	CORO_TEST("Move assign")
	{
		auto c1 = C1();
		c1();
		auto c2 = C1();
		REQUIRE(c1->m_Res == 1);
		REQUIRE(c2->m_Res == 0);

		c1 = std::move(c2);
		REQUIRE(c1->m_Res == 0);
		REQUIRE(c2->m_Res == 1);
	}

	CORO_TEST("Release")
	{
		auto c1 = C1();
		auto& p = *c1.operator->();
		auto handle = std::coroutine_handle<P1<int>>::from_promise(p);
		CHECK((c1.Release() == handle));
		handle.destroy();
		CHECK(!c1);
	}
} // namespace apollo::coro::ut
