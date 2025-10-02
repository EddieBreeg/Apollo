#pragma once

#include <PCH.hpp>

#include "Pixel.hpp"

namespace brk::rdr {
	/**
	 * A utility class to represent a view over bitmap data. Useful for bitmap operations such as
	 * copying, especially when dealing with subregions
	 */
	template <class Pixel>
	class BitmapView
	{
	public:
		using PixelTraits = PixelTraits<Pixel>;
		using ValueType = typename PixelTraits::ValueType;
		using PointerType = Pixel*;

		static constexpr uint32 Channels = PixelTraits::Channels;

		constexpr BitmapView() = default;
		constexpr ~BitmapView() = default;
		constexpr BitmapView(const BitmapView&) = default;

		/**
		 * \param data: The bitmapt's pixel data
		 * \param width: The width of the view. Can be small that of the bitmap it's referencing
		 * \param height: The height of the view. Can be small that of the bitmap it's referencing
		 * \param stride: The number of **pixels** from a pixel row to the next in the original. If
		 * 0, width will be used instead buffer
		 */
		constexpr BitmapView(Pixel* data, uint32 width, uint32 height, uint32 stride = 0) noexcept;
		/**
		 * \param data: The bitmapt's pixel data
		 * \param x: x-coordinate where the view region starts
		 * \param y: y-coordinate where the view region starts
		 * \param width: The width of the view. Can be small that of the bitmap it's referencing
		 * \param height: The height of the view. Can be small that of the bitmap it's referencing
		 * \param stride: The number of **pixels** from a pixel row to the next in the original. If
		 * 0, width will be used instead buffer
		 * \note: Equivalent to BitmapView(data + stride * y + x, width, height, stride)
		 */
		constexpr BitmapView(
			Pixel* data,
			uint32 x,
			uint32 y,
			uint32 width,
			uint32 height,
			uint32 stride = 0) noexcept;

		constexpr BitmapView& operator=(const BitmapView&) = default;

		constexpr void Swap(BitmapView& other) noexcept
		{
			std::swap(other.m_Data, m_Data);
			std::swap(other.m_Width, m_Width);
			std::swap(other.m_Height, m_Height);
			std::swap(other.m_Stride, m_Stride);
		}

		constexpr void SetWidth(uint32 w) noexcept { m_Width = w; }
		constexpr void SetHeight(uint32 h) noexcept { m_Height = h; }

		[[nodiscard]] constexpr uint32 GetWidth() const noexcept { return m_Width; }
		[[nodiscard]] constexpr uint32 GetHeight() const noexcept { return m_Height; }
		[[nodiscard]] constexpr uint32 GetStride() const noexcept { return m_Stride; }
		[[nodiscard]] constexpr uint32 GetByteStride() const noexcept
		{
			return m_Stride * sizeof(Pixel);
		}

		constexpr void Resize(uint32 width, uint32 height) noexcept
		{
			m_Width = width;
			m_Height = height;
			return *this;
		}

		[[nodiscard]] constexpr Pixel* GetData() const noexcept { return m_Data; }
		[[nodiscard]] constexpr Pixel* GetData(int32 xOffset, int32 yOffset) const noexcept
		{
			return m_Data + yOffset * m_Stride + xOffset;
		}

		constexpr Pixel& operator()(uint32 x, uint32 y) noexcept
		{
			return m_Data[x + y * m_Stride];
		}
		constexpr const Pixel& operator()(uint32 x, uint32 y) const noexcept
		{
			return m_Data[x + y * m_Stride];
		}

		[[nodiscard]] constexpr operator bool() const noexcept
		{
			return m_Data && m_Stride && m_Width && m_Height;
		}

		/**
		 * Copies the pixel data from this region to another.
		 * \param other: Destination bitmap region. Might point to another region of the same
		 * bitmap, or to another bitmap altogether
		 * \remark This function assumes that the destination region has at least the same number of
		 * pixels as the source. In general, it should be used to copy data from regions of same
		 * size, and could otherwise produce unexpected results
		 */
		constexpr void CopyTo(BitmapView& other) const;

	private:
		Pixel* m_Data = nullptr;
		uint32 m_Width = 0, m_Height = 0;
		size_t m_Stride = 0;
	};

	template <class Pixel>
	constexpr BitmapView<Pixel>::BitmapView(
		Pixel* data,
		uint32 width,
		uint32 height,
		uint32 stride) noexcept
		: m_Data(data)
		, m_Width(width)
		, m_Height(height)
		, m_Stride(stride ? stride : width)
	{}

	template <class Pixel>
	constexpr BitmapView<Pixel>::BitmapView(
		Pixel* data,
		uint32 x,
		uint32 y,
		uint32 width,
		uint32 height,
		uint32 stride) noexcept
		: BitmapView(data + x + stride * y, width, height, stride)
	{}

	template <class Pixel>
	constexpr void BitmapView<Pixel>::CopyTo(BitmapView<Pixel>& other) const
	{
		for (uint32 y = 0; y < m_Height; ++y)
		{
			for (uint32 x = 0; x < m_Width; ++x)
			{
				other(x, y) = m_Data[x + m_Stride * y];
			}
		}
	}
} // namespace brk::rdr