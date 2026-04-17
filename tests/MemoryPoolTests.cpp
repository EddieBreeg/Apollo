#include <catch2/catch_test_macros.hpp>
#include <core/Memory.hpp>
#include <list>
#include <memory_resource>
#include <vector>

#define MEMPOOL_TEST(name) TEST_CASE(name, "[memory_pool]")

namespace {
	struct Helper
	{
		Helper()
			: m_Tracker(m_Upstream)
		{}
		Helper(size_t initialSize)
			: m_Upstream(initialSize)
			, m_Tracker{ m_Upstream }
		{}
		Helper(void* init, size_t size)
			: m_Upstream(init, size, std::pmr::null_memory_resource())
			, m_Tracker{ m_Upstream }
		{}

		std::pmr::monotonic_buffer_resource m_Upstream;
		apollo::AllocationTracker<std::pmr::monotonic_buffer_resource> m_Tracker;
	};
} // namespace

static_assert(std::is_swappable_v<apollo::PoolAllocator<int>>);

namespace apollo::mempool_ut {

	MEMPOOL_TEST("GetBlockSize")
	{
		const MemoryPool pool{ 4 };
		CHECK(pool.GetBlockSize() == 4);
	}
	MEMPOOL_TEST("Compute block count")
	{
		const MemoryPool pool{ 4 };
		CHECK(pool.ComputeBlockCount(4) == 1);
		CHECK(pool.ComputeBlockCount(3) == 1);
		CHECK(pool.ComputeBlockCount(5) == 2);
	}
	MEMPOOL_TEST("Compute byte size")
	{
		const MemoryPool pool{ 4 };
		CHECK(pool.ComputeAllocationSize(4) == 4);
		CHECK(pool.ComputeAllocationSize(3) == 4);
		CHECK(pool.ComputeAllocationSize(5) == 8);
	}

	MEMPOOL_TEST("Allocate one block")
	{
		alignas(8) std::byte buf[128];
		Helper helper{ buf, sizeof(buf) };
		MemoryPool pool{ 4, 0, &helper.m_Tracker };

		void* ptr = pool.AllocateBlocks(1);
		CHECK(ptr);
		pool.DeallocateBlocks(ptr, 1);
	}

	MEMPOOL_TEST("Allocate one block, deallocate then reallocate one")
	{
		alignas(8) std::byte buf[128];
		Helper helper{ buf, sizeof(buf) };
		MemoryPool pool{ 4, 0, &helper.m_Tracker };

		void* const ptr = pool.AllocateBlocks(1);
		CHECK(ptr);
		pool.DeallocateBlocks(ptr, 1);
		void* const ptr2 = pool.AllocateBlocks(1);
		CHECK(ptr == ptr2);
	}

	MEMPOOL_TEST("Allocate one, then another, then 2")
	{
		alignas(8) std::byte buf[128];
		Helper helper{ buf, sizeof(buf) };
		MemoryPool pool{ 4, 2, &helper.m_Tracker };

		auto* ptr1 = static_cast<uint32*>(pool.AllocateBlocks(1));
		auto* ptr2 = static_cast<uint32*>(pool.AllocateBlocks(1));
		CHECK(ptr2 == (ptr1 + 1));
		{
			auto* ptr = pool.AllocateBlocks(2);
			REQUIRE(ptr);
			CHECK(ptr != ptr1);
			CHECK(ptr != ptr2);
		}
	}

	MEMPOOL_TEST("Clear")
	{
		alignas(8) std::byte buf[128];
		Helper helper{ buf, sizeof(buf) };
		MemoryPool pool{ 4, 1, &helper.m_Tracker };

		void* const ptr1 = pool.AllocateBlocks(1);
		REQUIRE(ptr1);
		void* const ptr2 = pool.AllocateBlocks(2);
		REQUIRE(ptr1);

		const size_t size = helper.m_Tracker.GetAllocatedSize();
		pool.Clear();

		CHECK(pool.AllocateBlocks(1) == ptr1);
		CHECK(pool.AllocateBlocks(2) == ptr2);
		CHECK(helper.m_Tracker.GetAllocatedSize() == size);
	}

	MEMPOOL_TEST("Release memory")
	{
		alignas(8) std::byte buf[128];
		Helper helper{ buf, sizeof(buf) };
		{
			MemoryPool pool{ 4, 2, &helper.m_Tracker };
			CHECK(helper.m_Tracker.GetAllocatedSize());
		}
		CHECK(helper.m_Tracker.GetAllocatedSize() == 0);
	}

	MEMPOOL_TEST("Allocate 2 blocks via allocator")
	{
		uint64 buf[32];
		Helper helper{ buf, sizeof(buf) };
		MemoryPool pool{ 4, 2, &helper.m_Tracker };

		PoolAllocator<int32> alloc = pool.GetAllocator<int32>();
		int32* const ptr = alloc.allocate(2);
		REQUIRE(ptr);
		alloc.deallocate(ptr, 2);

		int32* const ptr2 = alloc.allocate(2);
		CHECK(ptr == ptr2);
	}

	MEMPOOL_TEST("Build vector of 3 integers")
	{
		alignas(8) std::byte buf[128];
		Helper helper{ buf, sizeof(buf) };
		MemoryPool pool{ 4, 0, &helper.m_Tracker };
		const int* ptr = nullptr;

		size_t size = helper.m_Tracker.GetAllocatedSize();
		REQUIRE(size == 0);
		{
			std::vector<int, PoolAllocator<int>> vec{ pool.GetAllocator<int>() };
			vec.reserve(3);

			size = helper.m_Tracker.GetAllocatedSize();
			vec.emplace_back(0);
			vec.emplace_back(1);
			vec.emplace_back(2);
			ptr = vec.data();
			REQUIRE(helper.m_Tracker.GetAllocatedSize() == size);
		}
		CHECK(helper.m_Tracker.GetAllocatedSize() == size);
		CHECK(ptr[0] == 0);
		CHECK(ptr[1] == 1);
		CHECK(ptr[2] == 2);
	}

	MEMPOOL_TEST("Vector for ints")
	{
		static constexpr int32 n = 100;
		AllocationTracker<> tracker{ *std::pmr::new_delete_resource() };
		MemoryPool pool{ 4, 0, &tracker };
		std::vector<int, PoolAllocator<int>> vec{ pool.GetAllocator<int>() };
		for (int i = 0; i < n; ++i)
		{
			vec.emplace_back(i);
		}
	}

	MEMPOOL_TEST("Allocate 3 integers")
	{
		alignas(8) uint32 buf[128] = {};
		Helper helper{ buf, sizeof(buf) };
		uint32* start = nullptr;
		{
			MemoryPool pool{ 4, 3, &helper.m_Tracker };

			start = (uint32*)pool.AllocateBlocks(1);
			*start = 0;

			{
				uint32* p = (uint32*)pool.AllocateBlocks(1);
				CHECK(*start == 0);
				*p = 1;
			}
			{
				uint32* p = (uint32*)pool.AllocateBlocks(1);
				CHECK(*start == 0);
				CHECK(start[1] == 1);
				*p = 2;
			}
			for (auto* it = start; it != (start + 3); ++it)
			{
				pool.DeallocateBlocks(it, 1);
				CHECK(start[0] == 0);
				CHECK(start[1] == 1);
				CHECK(start[2] == 2);
			}
		}
		CHECK(start[0] == 0);
		CHECK(start[1] == 1);
		CHECK(start[2] == 2);
	}

	MEMPOOL_TEST("std::list<int>")
	{
		uint64 buf[128] = {};
		Helper helper{ buf, sizeof(buf) };
		MemoryPool pool{ 4, 3, &helper.m_Tracker };
		std::list<int, PoolAllocator<int>> list{ pool.GetAllocator<int>() };
		for (int i = 0; i < 10; ++i)
		{
			list.emplace_back(i);
		}
	}
} // namespace apollo::mempool_ut

#undef MEMPOOL_TEST