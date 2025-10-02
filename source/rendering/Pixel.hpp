#pragma once

#include <PCH.hpp>

namespace brk::rdr {
	template <class T, uint32 N>
	struct Pixel;

	template <class T>
	struct Pixel<T, 1>
	{
		T r = 0;
	};

	template <class T>
	struct Pixel<T, 2>
	{
		T r = 0, g = 0;
	};

	template <class T>
	struct Pixel<T, 3>
	{
		T r = 0, g = 0, b = 0;
	};

	template <class T>
	struct Pixel<T, 4>
	{
		T r = 0, g = 0, b = 0, a = 0;
	};

	template <class T>
	using RPixel = Pixel<T, 1>;

	template <class T>
	using RGPixel = Pixel<T, 2>;

	template <class T>
	using RGBPixel = Pixel<T, 3>;

	template <class T>
	using RGBAPixel = Pixel<T, 4>;

	template <class P>
	struct PixelTraits;

	template <class T, uint32 N>
	struct PixelTraits<Pixel<T, N>>
	{
		using ValueType = T;
		static constexpr uint32 Channels = N;
	};

	template <class P>
	struct PixelTraits<const P>
	{
		using ValueType = typename PixelTraits<P>::ValueType const;
		static constexpr uint32 Channels = PixelTraits<P>::Channels;
	};

	template <class T>
	[[nodiscard]] constexpr bool operator==(const Pixel<T, 1> a, const Pixel<T, 1> b) noexcept
	{
		return a.r == b.r;
	}
	template <class T>
	[[nodiscard]] constexpr bool operator==(const Pixel<T, 2> a, const Pixel<T, 2> b) noexcept
	{
		return a.r == b.r && a.g == b.g;
	}
	template <class T>
	[[nodiscard]] constexpr bool operator==(const Pixel<T, 3> a, const Pixel<T, 3> b) noexcept
	{
		return a.r == b.r && a.g == b.g && a.b == b.b;
	}
	template <class T>
	[[nodiscard]] constexpr bool operator==(const Pixel<T, 4> a, const Pixel<T, 4> b) noexcept
	{
		return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
	}

	template<class T, uint32 N>
	[[nodiscard]] constexpr bool operator!=(const Pixel<T, N> a, const Pixel<T, N> b)
	{
		return !(a == b);
	}
} // namespace brk::rdr