#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#define INT_DECL(type) using type = type##_t
INT_DECL(int8);
INT_DECL(int16);
INT_DECL(int32);
INT_DECL(int64);
INT_DECL(uint8);
INT_DECL(uint16);
INT_DECL(uint32);
INT_DECL(uint64);
#undef INT_DECL

using float2 = glm::vec<2, float>;
using float3 = glm::vec<3, float>;
using float4 = glm::vec<4, float>;

namespace apollo {
	[[nodiscard]] constexpr auto Min(auto&& a, auto&& b) noexcept(noexcept(a < b))
	{
		if (a < b)
			return a;
		return b;
	}

	[[nodiscard]] constexpr auto Max(auto&& a, auto&& b) noexcept(noexcept(a < b))
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

	[[nodiscard]] constexpr uint32_t Padding(uint32_t offset, uint32_t alignment) noexcept
	{
		const uint32_t mask = alignment - 1;
		return (alignment - (offset & mask)) & mask;
	}

	[[nodiscard]] constexpr uint32_t Align(uint32_t offset, uint32_t alignment) noexcept
	{
		return offset + Padding(offset, alignment);
	}

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