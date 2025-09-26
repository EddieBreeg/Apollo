#include <catch2/catch_test_macros.hpp>
#include <core/Queue.hpp>

#define QUEUE_TEST_CASE(name) TEST_CASE(name, "[queue][containers]")

namespace brk::containers::ut {
	QUEUE_TEST_CASE("Empty Queue")
	{
		const Queue<int> q;
		CHECK(q.GetSize() == 0);
		CHECK(q.GetCapacity() == 0);
	}

	QUEUE_TEST_CASE("Add 1 element")
	{
		Queue<int> q;
		REQUIRE(q.GetSize() == 0);
		q.Add(1);
		CHECK(q.GetSize() == 1);
		CHECK(q.GetCapacity() >= 1);
	}

	QUEUE_TEST_CASE("GetFront")
	{
		Queue<uint32> q;
		q.Add(1u);
		REQUIRE(q.GetSize() == 1);
		CHECK(q.GetFront() == 1);
	}

	QUEUE_TEST_CASE("Add then PopAndGetFront")
	{
		Queue<uint32> q;
		REQUIRE(q.GetSize() == 0);
		q.Add(1u);
		REQUIRE(q.GetSize() == 1);
		CHECK(q.PopAndGetFront() == 1);
		CHECK(q.GetSize() == 0);

		q.Add(1u);
		q.Add(2u);
		REQUIRE(q.GetSize() == 2);
		CHECK(q.PopAndGetFront() == 1);
		CHECK(q.PopAndGetFront() == 2);
	}

	QUEUE_TEST_CASE("Construct with initial capacity")
	{
		const Queue<uint32> q{ 2 };
		CHECK(q.GetCapacity() == 2);
	}

	QUEUE_TEST_CASE("Reserve")
	{
		Queue<uint32> q;
		q.Reserve(2);
		CHECK(q.GetCapacity() == 2);
	}

	QUEUE_TEST_CASE("Pop Front")
	{
		Queue<uint32> q{ 1 };
		q.PopFront(); // no-op
		CHECK(q.GetSize() == 0);
		q.Add(1u);
		q.PopFront();
		CHECK(q.GetSize() == 0);
	}

	QUEUE_TEST_CASE("Pop and Get Front on empty queue")
	{
		Queue<int> q;
		CHECK_THROWS_AS(q.PopAndGetFront(), std::runtime_error);
	}

	QUEUE_TEST_CASE("Fill, pop one then add one")
	{
		Queue<int32> q{ 2 };
		REQUIRE(q.GetCapacity() == 2);
		for (int32 i = 0; i < 2; ++i)
		{
			q.Add(i);
		}
		REQUIRE(q.GetSize() == 2);
		REQUIRE(q.GetCapacity() == 2);
		q.PopFront();
		q.Add(666);
		CHECK(q.GetCapacity() == 2);

		CHECK(q.GetFront() == 1);
	}

	QUEUE_TEST_CASE("Clear")
	{
		Queue<int32> q{ 1 };
		q.Add(1);
		q.Clear();
		CHECK(q.GetSize() == 0);
		CHECK(q.GetCapacity() == 1);
	}

	QUEUE_TEST_CASE("Copy")
	{
		SECTION("Copy Construction")
		{
			Queue<int32> q1{ 2 };
			q1.Add(1);
			q1.Add(2);
			Queue<int32> q2{ q1 };
			CHECK(q2.GetSize() == 2);
			CHECK(q2.PopAndGetFront() == 1);
			CHECK(q2.PopAndGetFront() == 2);
		}
		SECTION("Copy assign to empty")
		{
			Queue<int32> q1{ 2 };
			q1.Add(1);
			q1.Add(2);
			Queue<int32> q2;
			q2 = q1;

			CHECK(q2.GetSize() == 2);
			CHECK(q2.PopAndGetFront() == 1);
			CHECK(q2.PopAndGetFront() == 2);
		}
		SECTION("Copy assign to smaller size")
		{
			Queue<int32> q1{ 2 };
			q1.Add(1);
			q1.Add(2);
			Queue<int32> q2{ 10 };
			q2.Add(666);
			q2 = q1;

			CHECK(q2.GetSize() == 2);
			CHECK(q2.GetCapacity() == 10);
			CHECK(q2.PopAndGetFront() == 1);
			CHECK(q2.PopAndGetFront() == 2);
		}
		SECTION("Copy assign to bigger size")
		{
			Queue<int32> q1{ 2 };
			q1.Add(1);
			q1.Add(2);
			Queue<int32> q2{ 10 };
			q2.Add(4);
			q2.Add(5);
			q2.Add(6);
			q2 = q1;

			CHECK(q2.GetSize() == 2);
			CHECK(q2.GetCapacity() == 10);
			CHECK(q2.PopAndGetFront() == 1);
			CHECK(q2.PopAndGetFront() == 2);
		}
	}
	QUEUE_TEST_CASE("Move")
	{
		SECTION("Move Construction")
		{
			Queue<int32> q1{ 1 };
			q1.Add(1);
			Queue<int32> q2{ std::move(q1) };

			CHECK(q1.GetSize() == 0);
			CHECK(q1.GetCapacity() == 0);
			CHECK(q2.GetSize() == 1);
			CHECK(q2.GetCapacity() == 1);
			CHECK(q2.GetFront() == 1);
		}
		SECTION("Move assignment to smaller empty")
		{
			Queue<int32> q1{ 1 };
			q1.Add(1);
			Queue<int32> q2;
			q2 = std::move(q1);

			CHECK(q2.GetSize() == 1);
			CHECK(q2.GetCapacity() == 1);
			CHECK(q2.GetFront() == 1);
		}
		SECTION("Move assignment to bigger queue")
		{
			Queue<int32> q1{ 1 };
			q1.Add(1);
			Queue<int32> q2{ 3 };
			q2.Add(4);
			q2.Add(5);
			q2.Add(6);
			q2 = std::move(q1);

			CHECK(q2.GetSize() == 1);
			CHECK(q2.GetCapacity() == 1);
			CHECK(q2.GetFront() == 1);
		}
	}

	QUEUE_TEST_CASE("Swap")
	{
		Queue<int32> q1{ 1 }, q2{ 2 };
		q1.Add(1);
		q2.Add(2);
		q1.Swap(q2);
		CHECK(q1.GetFront() == 2);
		CHECK(q2.GetFront() == 1);
	}
} // namespace brk::containers::ut

#undef QUEUE_TEST_CASE