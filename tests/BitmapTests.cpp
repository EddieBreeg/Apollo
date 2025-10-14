#include <catch2/catch_test_macros.hpp>
#include <rendering/Bitmap.hpp>

namespace apollo::rdr::bitmap_ut {
#define BITMAP_TEST(name) TEST_CASE(name, "[bitmap]")

	using RGBA8Pixel = RGBAPixel<uint8>;
	using R8Pixel = RPixel<uint8>;

	BITMAP_TEST("Null test")
	{
		constexpr BitmapView<const RGBA8Pixel> bitmap;
		static_assert(!bitmap);
		static_assert(bitmap.GetData() == nullptr);
		static_assert(bitmap.GetWidth() == 0);
		static_assert(bitmap.GetHeight() == 0);
		static_assert(bitmap.GetStride() == 0);
		static_assert(bitmap.GetByteStride() == 0);
	}

	BITMAP_TEST("Non-empty tests")
	{
		static constexpr RGBA8Pixel pixels[2 * 2]{ { 0 }, { 1 }, { 2 }, { 3 } };
		constexpr BitmapView<const RGBA8Pixel> view{ pixels, 2, 2 };
		static_assert(view.GetWidth() == 2);
		static_assert(view.GetHeight() == 2);
		static_assert(view.GetStride() == 2);
		static_assert(view.GetByteStride() == 8);
	}

	BITMAP_TEST("Get pixel values")
	{
		static constexpr RGBA8Pixel pixels[2 * 2]{ { 0 }, { 1 }, { 2 }, { 3 } };
		constexpr BitmapView<const RGBA8Pixel> view{ pixels, 2, 2 };
		static_assert(view(0, 0) == RGBA8Pixel{});
		static_assert(view(1, 0) == RGBA8Pixel{ 1 });
		static_assert(view(0, 1) == RGBA8Pixel{ 2 });
		static_assert(view(1, 1) == RGBA8Pixel{ 3 });
	}
	BITMAP_TEST("Construction with offset")
	{
		static constexpr RGBA8Pixel pixels[2 * 3]{
			{ 0 },
			{ 1 },
			{ 2 },
			{ 3 },
		};
		constexpr BitmapView view{ pixels, 1, 1, 1, 1, 2 };
		static_assert(view(0, 0) == RGBA8Pixel{ 3 });
	}
	BITMAP_TEST("Get pixel address")
	{
		static constexpr RGBA8Pixel pixels[2];
		constexpr BitmapView view{ pixels, 2, 1 };
		static_assert(view.GetData() == pixels);
		static_assert(view.GetData(1, 0) == (pixels + 1));
	}
	BITMAP_TEST("Set pixel values")
	{
		R8Pixel pixels[4]{};
		BitmapView<R8Pixel> view{ pixels, 2, 2 };
		view(0, 0) = R8Pixel{ 1 };
		view(1, 0) = R8Pixel{ 2 };
		view(0, 1) = R8Pixel{ 3 };
		view(1, 1) = R8Pixel{ 4 };
		CHECK(pixels[0] == R8Pixel{ 1 });
		CHECK(pixels[1] == R8Pixel{ 2 });
		CHECK(pixels[2] == R8Pixel{ 3 });
		CHECK(pixels[3] == R8Pixel{ 4 });
	}

	BITMAP_TEST("Copy test")
	{
		R8Pixel buf1[4] = { { 1 }, { 2 }, { 3 }, { 4 } };
		R8Pixel buf2[16];
		BitmapView v1{ buf1, 2, 2 };
		BitmapView v2{ buf2, 1, 1, 2, 2, 4 };
		v1.CopyTo(v2);

		for (uint32 j = 0; j < v2.GetHeight(); ++j)
		{
			for (uint32 i = 0; i < v2.GetWidth(); ++i)
			{
				CHECK(v1(i, j) == v2(i, j));
			}
		}
		const BitmapView<const R8Pixel> v3{ buf2, 4, 4 };
		for (uint32 j = 0; j < v3.GetHeight(); ++j)
		{
			for (uint32 i = 0; i < v3.GetWidth(); ++i)
			{
				CHECK(v3(i, j) == buf2[i + 4 * j]);
			}
		}
	}
} // namespace apollo::rdr::bitmap_ut