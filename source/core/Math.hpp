#pragma once

/** \file Math.hpp */

#include <cstdint>
#if APOLLO_RUNTIME
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#endif

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

#if APOLLO_RUNTIME
using float2 = glm::vec<2, float>;
using float3 = glm::vec<3, float>;
using float4 = glm::vec<4, float>;
#endif

namespace apollo {
	[[nodiscard]] constexpr auto Min(auto&& a, auto&& b) noexcept(noexcept(a < b))
	{
		if (a < b)
			return a;
		return b;
	}

	[[nodiscard]] constexpr auto Max(auto&& a, auto&& b) noexcept(noexcept(a > b))
	{
		if (a > b)
			return a;
		return b;
	}

	template <class T, class U, class V>
	[[nodiscard]] constexpr auto Clamp(T&& v, U&& min, V&& max) noexcept(
		noexcept(v < min) && noexcept(v > max))
	{
		if (v < min)
			return min;
		if (v > max)
			return max;
		return v;
	}

	[[nodiscard]] constexpr bool IsPowerOfTwo(auto&& val) noexcept
	{
		return (val & (~val + 1)) == val;
	}

	/**
	 * \brief Computes the padding required to align an offset on some boundary
	 * \param offset: The value to align
	 * \param alignment: The alignment to use. Must be a power of 2
	 * \returns The padding value
	 * \warning The result is undefined if `alignment` is not a power of two
	 */
	[[nodiscard]] constexpr uint32_t Padding(uint32_t offset, uint32_t alignment) noexcept
	{
		const uint32_t mask = alignment - 1;
		return (alignment - (offset & mask)) & mask;
	}

	/**
	 * \brief Aligns an offset on the provided boundary
	 * \param offset: The value to align
	 * \param alignment: The alignment to use. Must be a power of 2
	 * \returns offset + \ref apollo::Padding "Padding"(offset, alignment)
	 * \warning The result is undefined if `alignment` is not a power of two
	 */
	[[nodiscard]] constexpr uint32_t Align(uint32_t offset, uint32_t alignment) noexcept
	{
		return offset + Padding(offset, alignment);
	}

	/**
	* \brief Maps a value linearly from a source range to a destination range
	* \param x: The value to map
	* \param fromMin: Start of the source range
	* \param fromMax: End of the source range
	* \param toMin: Start of the destination range
	* \param toMax: End of the destination range
	 */
	[[nodiscard]] constexpr float MapRange(
		float x,
		float fromMin,
		float fromMax,
		float toMin,
		float toMax)
	{
		return (x - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
	}
} // namespace apollo