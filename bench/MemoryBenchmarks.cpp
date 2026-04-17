#include <benchmark/benchmark.h>
#include <core/Memory.hpp>

namespace {
	static constexpr int32 g_ElemCount = 1000;

	void List_New(benchmark::State& state)
	{
		for (auto&& _ : state)
		{
			std::list<int> l;
			for (int i = 0; i < g_ElemCount; ++i)
			{
				l.emplace_back(i);
			}
		}
	}
	void List_MemoryPool(benchmark::State& state, apollo::MemoryPool pool)
	{
		for (auto&& _ : state)
		{
			std::list<int, apollo::PoolAllocator<int>> l{ pool.GetAllocator<int>() };
			for (int i = 0; i < g_ElemCount; ++i)
			{
				l.emplace_back(i);
			}
		}
	}

	void Vector_New(benchmark::State& state, int32 n)
	{
		for (auto&& _ : state)
		{
			std::vector<int> v;
			while (n)
			{
				v.emplace_back(n--);
			}
		}
	}
	void Vector_MemoryPool(benchmark::State& state, int32 n, apollo::MemoryPool pool)
	{
		for (auto&& _ : state)
		{
			std::vector<int, apollo::PoolAllocator<int>> v{ pool.GetAllocator<int>() };
			while (n)
			{
				v.emplace_back(n--);
			}
		}
	}

	void VectorWithReserve_New(benchmark::State& state, int32 n)
	{
		for (auto&& _ : state)
		{
			std::vector<int> v;
			v.reserve(n);
			while (n)
			{
				v.emplace_back(n--);
			}
		}
	}
	void VectorWithReserve_MemoryPool(benchmark::State& state, int32 n, apollo::MemoryPool pool)
	{
		for (auto&& _ : state)
		{
			std::vector<int, apollo::PoolAllocator<int>> v{ pool.GetAllocator<int>() };
			v.reserve(n);
			while (n)
			{
				v.emplace_back(n--);
			}
		}
	}

	[[maybe_unused]] void Fragmentation_New(benchmark::State& state, int32 n)
	{
		for (auto&& _ : state)
		{
			std::list<int> l;
			for (int32 i = 0; i < n; ++i)
			{
				l.emplace_back(i++);
				auto it = --l.end();
				l.emplace_back(i++);
				l.emplace_back(i);
				l.erase(it);
			}
		}
	}
	[[maybe_unused]] void Fragmentation_Pool(
		benchmark::State& state,
		int32 n,
		apollo::MemoryPool pool)
	{
		for (auto&& _ : state)
		{
			std::list<int, apollo::PoolAllocator<int>> l{ pool.GetAllocator<int>() };
			for (int32 i = 0; i < n; ++i)
			{
				l.emplace_back(i++);
				auto it = --l.end();
				l.emplace_back(i++);
				l.emplace_back(i);
				l.erase(it);
			}
		}
	}
} // namespace

BENCHMARK_CAPTURE(Vector_New, "NoReserve", 100);
BENCHMARK_CAPTURE(
	Vector_MemoryPool,
	"Empty Pool NoReserve",
	100,
	apollo::MemoryPool{ sizeof(int), 150 });

BENCHMARK_CAPTURE(VectorWithReserve_New, "Reserve", 100);
BENCHMARK_CAPTURE(
	VectorWithReserve_MemoryPool,
	"Empty Pool Reserve",
	100,
	apollo::MemoryPool{ sizeof(int), 150 });

BENCHMARK(List_New);
BENCHMARK_CAPTURE(List_MemoryPool, "Initalized Pool", apollo::MemoryPool{ 24, g_ElemCount });

BENCHMARK_CAPTURE(Fragmentation_New, "Fragmented New", g_ElemCount);
BENCHMARK_CAPTURE(
	Fragmentation_Pool,
	"Fragmented Pool",
	g_ElemCount,
	apollo::MemoryPool{ 24, g_ElemCount });