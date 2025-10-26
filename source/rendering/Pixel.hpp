#pragma once

#include <PCH.hpp>

namespace apollo::rdr {
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

	template <class T, uint32 N>
	[[nodiscard]] constexpr bool operator!=(const Pixel<T, N> a, const Pixel<T, N> b)
	{
		return !(a == b);
	}

	enum class EPixelFormat : int8
	{
		Invalid = -1,

		// unsigned 8-bit formats
		R8_UNorm,
		RG8_UNorm,
		RGBA8_UNorm,

		// signed 8-bit formats
		R8_SNorm,
		RG8_SNorm,
		RGBA8_SNorm,

		// Depth/Stencil Formats
		Depth16,
		Depth24,
		Depth32,
		Depth24_Stencil8,
		Depth32_Stencil8,

		NFormats
	};

} // namespace apollo::rdr