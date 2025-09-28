#include <catch2/catch_test_macros.hpp>
#include <core/TypeInfo.hpp>

namespace {
	uint32 g_IntIndex;
	const brk::TypeInfo* g_IntInfo = &brk::TypeInfo::Get<int>();

	struct Gen
	{
		static inline uint32 s_Index = 0;
		static uint32 GetNext() noexcept { return s_Index++; }
	};

	template <class T>
	using TypeIndexT = brk::TypeIndex<T, Gen>;
} // namespace

TEST_CASE("Sequential index", "[rtti]")
{
	const uint32 i0 = brk::TypeIndex<int>{};
	const uint32 i1 = brk::TypeIndex<unsigned int>{};

	CHECK(i1 == (i0 + 1));

	g_IntIndex = i0;
}

TEST_CASE("Index using new generator", "[rtti]")
{
	const uint32 i = TypeIndexT<int>{};
	CHECK(i == g_IntIndex);
}

TEST_CASE("Type info test", "[rtti]")
{
	CHECK(g_IntInfo->m_Index == g_IntIndex);
	CHECK(g_IntInfo->m_Size == sizeof(int));
	CHECK(g_IntInfo->m_Alignment == alignof(int));
}

TEST_CASE("Type info consistency", "[rtti]")
{
	const brk::TypeInfo& info = brk::TypeInfo::Get<int>();
	CHECK(info == *g_IntInfo);
}