#pragma once

#include "PCH.hpp"

#include <string_view>

namespace brk
{
	template<class T>
	struct Hash;

	struct StringHash
	{
		explicit constexpr StringHash(const std::string_view str) noexcept
		{
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
}