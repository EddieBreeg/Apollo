#include <catch2/catch_template_test_macros.hpp>
#include <core/Hash.hpp>

namespace brk::hash::ut {
	static_assert(brk::Hasher<brk::Hash<int>, int>);
	static_assert(brk::Hashable<brk::StringHash>);
	static_assert(brk::Hasher<brk::Hash<brk::StringHash>, brk::StringHash>);

	template <std::integral Int>
	consteval bool IntHashTest(Int x)
	{
		using Uint = std::make_unsigned_t<Int>;
		return Hash<Int>{}(x) == static_cast<Uint>(x);
	}

	template <class T>
	bool PointerHashTest(T* const ptr)
	{
		return Hash<T*>{}(ptr) == (uintptr_t)ptr;
	}

#define HASH_TEST(name) TEST_CASE(name, "[hash]")

	HASH_TEST("Integer hashing")
	{
		static_assert(IntHashTest<char>('a'));
		static_assert(IntHashTest<unsigned char>('a'));
		static_assert(IntHashTest<int16>(1));
		static_assert(IntHashTest<uint16>(1));
		static_assert(IntHashTest<int32>(3));
		static_assert(IntHashTest<uint32>(3));
		static_assert(IntHashTest<int64>(3));
		static_assert(IntHashTest<uint64>(3));

		CHECK(PointerHashTest<void>(nullptr));
		CHECK(PointerHashTest((const int*)1ull));
	}

	template <class T>
	consteval uint64 Combine(uint64 h, T x)
	{
		return (h ^= 0x90e14f3da127aed8 + Hash<T>{}(x) + (h << 6) + (h >> 2));
	}

	HASH_TEST("Combining hashes")
	{
		static_assert(HashCombine(0, 1, 2, 3) == Combine(Combine(Combine(0, 1), 2), 3));
		constexpr Hash<int32> hasher;
		static_assert(HashCombine(0, hasher, 1, 2, 3) == Combine(Combine(Combine(0, 1), 2), 3));
	}
} // namespace brk::hash::ut