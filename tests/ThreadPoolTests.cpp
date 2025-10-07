#include <catch2/catch_test_macros.hpp>
#include <core/ThreadPool.hpp>
#include <semaphore>

#define THREADPOOL_TEST(name) TEST_CASE(name, "[mt][thread_pool]")

namespace brk::mt::ut {
	using namespace std::chrono_literals;

	THREADPOOL_TEST("GetThreadCount test")
	{
		SECTION("Default thread count")
		{
			ThreadPool tp;
			CHECK(tp.GetThreadCount() == ThreadPool::DefaultThreadCount);
		}
		SECTION("Non-default thread count")
		{
			ThreadPool tp{ 2 };
			CHECK(tp.GetThreadCount() == 2);
		}
	}

	THREADPOOL_TEST("Enqueue")
	{
		SECTION("Single job on 1 thread")
		{
			std::counting_semaphore semaphore{ 0 };
			bool result = false;
			ThreadPool tp{ 1 };
			tp.Enqueue(
				[&]()
				{
					result = true;
					semaphore.release();
				});

			CHECK(semaphore.try_acquire_for(300ms));
			CHECK(result);
		}
		SECTION("2 jobs on 1 thread")
		{
			std::counting_semaphore sem1{ 0 };
			std::counting_semaphore sem2{ 0 };
			ThreadPool tp{ 1 };
			REQUIRE(tp.GetThreadCount() == 1);
			tp.Enqueue(
				[&]()
				{
					sem1.release();
				});
			tp.Enqueue(
				[&]()
				{
					sem1.acquire();
					sem2.release();
				});

			CHECK(sem2.try_acquire_for(300ms));
		}
		SECTION("2 jobs on 2 threads")
		{
			std::counting_semaphore sem1{ 0 };
			std::counting_semaphore sem2{ 0 };
			ThreadPool tp{ 2 };
			REQUIRE(tp.GetThreadCount() == 2);
			tp.Enqueue(
				[&]()
				{
					sem1.acquire();
					sem2.release();
				});
			tp.Enqueue(
				[&]()
				{
					sem1.release();
				});
			CHECK(sem2.try_acquire_for(300ms));
		}
		SECTION("Job with arguments")
		{
			std::binary_semaphore semaphore{ 0 };
			ThreadPool tp{ 1 };
			int32 result = 0;
			tp.Enqueue(
				[](std::binary_semaphore& sem, int32& out_result)
				{
					out_result = 1;
					sem.release();
				},
				std::ref(semaphore),
				std::ref(result));
			CHECK(semaphore.try_acquire_for(300ms));
			CHECK(result == 1);
		}
	}

	THREADPOOL_TEST("EnqueueAndGetFuture")
	{
		SECTION("future<void>")
		{
			int32 result = 0;
			ThreadPool tp{ 1 };
			std::future future = tp.EnqueueAndGetFuture(
				[&](int32 val)
				{
					result = val;
				},
				2);
			future.get();
			CHECK(result == 2);
		}
		SECTION("future<int32>")
		{
			ThreadPool tp{ 1 };
			std::future result = tp.EnqueueAndGetFuture(
				[](int32 val)
				{
					return val;
				},
				3);
			CHECK(result.get() == 3);
		}
	}

	THREADPOOL_TEST("Broken promise")
	{
		std::binary_semaphore sem{ 0 };
		std::future<void> result;
		{
			ThreadPool tp{ 1 };
			tp.Enqueue(
				[&]()
				{
					sem.acquire();
				});
			result = tp.EnqueueAndGetFuture([]() {});
			tp.Stop();
			sem.release();
		}

		CHECK_THROWS_AS(result.get(), std::future_error);
	}
} // namespace brk::mt::ut