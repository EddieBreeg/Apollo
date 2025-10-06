#pragma once

#include "PCH.hpp"

#include <string_view>

namespace brk {
	template <class T>
	struct Hash;

	/**
	 * This utility class just stores a 32-bit hash value, computed from a string.
	 * Useful when we might want to access a hashmap using either the string itself,
	 * or its pre-computed hash value.
	 */
	struct StringHash
	{
		explicit constexpr StringHash(const std::string_view str) noexcept
		{
			/* Modified version of the Jenkins-One-At-A-Time hash. Not the best hash in the world,
			 * but good enough for short text strings and importantly: constexpr friendly.
			 * You can find the original algorithm here:
			 * https://en.wikipedia.org/wiki/Jenkins_hash_function#one_at_a_time
			 */
			for (const char c : str)
			{
				m_Hash += c;
				m_Hash += m_Hash << 10;
				m_Hash ^= m_Hash >> 6;
			}
			m_Hash *= 0x68f96d6d;
			m_Hash ^= m_Hash >> 7;
			m_Hash *= m_Hash << 3;
			m_Hash ^= m_Hash >> 11;
			m_Hash += m_Hash << 15;
		}
		/**
		 * Constructs the object from a pre-computed string hash value. This assumes the hash was
		 * computed using the same algorithm as above
		 */
		constexpr StringHash(const uint32 h) noexcept
			: m_Hash(h)
		{}

		[[nodiscard]] constexpr operator uint32() const noexcept { return m_Hash; }

	private:
		uint32 m_Hash = 0;
	};

	template <>
	struct Hash<StringHash>
	{
		[[nodiscard]] constexpr uint32 operator()(StringHash h) const noexcept { return h; }
	};

	namespace string_hash_literals {
		consteval StringHash operator""_strh(const char* str) noexcept
		{
			return StringHash{ str };
		}
	} // namespace string_hash_literals
} // namespace brk