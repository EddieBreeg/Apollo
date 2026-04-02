#include <catch2/catch_test_macros.hpp>
#include <core/Blob.hpp>

#define BLOB_TEST(name) TEST_CASE(name, "[blob]")

namespace {
	struct Blob1 final : public apollo::Blob
	{
		Blob1(void* p, size_t n) noexcept
			: apollo::Blob(p, n)
		{
			++s_Count;
		}
		~Blob1() noexcept { --s_Count; }

		static inline uint32 s_Count = 0;
	};

	struct Blob2 final : public apollo::Blob
	{
		Blob2(void* ptr, size_t n, int val)
			: apollo::Blob(ptr, n)
		{
			new (m_Ptr) int{ val };
		}

		int& GetVal() noexcept { return *GetPtrAs<int>(); }
		int GetVal() const noexcept { return *GetPtrAs<int>(); }
	};
} // namespace

namespace apollo::blob_ut {
	BLOB_TEST("Simple alloc")
	{
		Blob1* b = Blob::Allocate<Blob1>(8);
		CHECK(Blob1::s_Count == 1);
		delete b;
		CHECK(Blob1::s_Count == 0);
	}
	BLOB_TEST("GetSize()")
	{
		std::unique_ptr b{ Blob::Allocate(8) };
		CHECK(b->GetSize() == 8);
	}
	BLOB_TEST("GetPtr()")
	{
		std::unique_ptr b{ Blob::Allocate(8) };
		CHECK(b->GetPtr() == (b.get() + 1));
	}
	BLOB_TEST("GetPtrAs<int>()")
	{
		std::unique_ptr<Blob2> ptr{ Blob2::Allocate<Blob2>(sizeof(int), 1) };
		CHECK(ptr->GetVal() == 1);
	}
} // namespace apollo::blob_ut