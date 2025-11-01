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

		// unsigned normalized float
		A8_UNorm = 1,
		R8_UNorm,
		RG8_UNorm,
		RGBA8_UNorm,
		R16_UNorm,
		RG16_UNorm,
		RGBA16_UNorm,
		RGB10A2_UNorm,
		B5G6R5_UNorm,
		BGR5A1_UNorm,
		BGRA4_UNorm,
		BGRA8_UNorm,

		// signed normalized float formats
		R8_SNorm = 21,
		RG8_SNorm,
		RGBA8_SNorm,
		R16_SNorm,
		RG16_SNorm,
		RGBA16_SNorm,
		// signed float formats
		R16_Float,
		RG16_Float,
		RGBA16_Float,
		R32_Float,
		RG32_Float,
		RGBA32_Float,

		RG11B10_UFloat,

		// UInt formats
		R8_Uint,
		RG8_Uint,
		RGBA8_Uint,
		R16_Uint,
		RG16_Uint,
		RGBA16_Uint,
		R32_Uint,
		RG32_Uint,
		RGBA32_Uint,

		// Int formats
		R8_Int,
		RG8_Int,
		RGBA8_Int,
		R16_Int,
		RG16_Int,
		RGBA16_Int,
		R32_Int,
		RG32_Int,
		RGBA32_Int,

		// sRGB unsigned normalized formats
		RGBA8_UNorm_SRGB,
		BGRA8_UNorm_SRGB,

		// Depth/Stencil Formats
		Depth16 = 58,
		Depth24,
		Depth32,
		Depth24_Stencil8,
		Depth32_Stencil8,

		NFormats
	};

} // namespace apollo::rdr