#pragma once

#include "PCH.hpp"

#include <spdlog/fmt/bundled/base.h>
#include <string_view>

/** \file StringHash.hpp */

namespace apollo {
	template <class T>
	struct Hash;

	/**
	 * \brief Pre-hashed string utility

	 * This utility class stores both a reference to the string (as an std::string_view) and the
	 pre-computed hash, for easy lookups in maps and such
	 */
	struct HashedString
	{
		constexpr HashedString(std::string_view str) noexcept
			: m_Str{ str }
			, m_Hash{ ComputeHash(str, 0) }
		{}

		template <size_t N>
		consteval HashedString(const char (&str)[N]) noexcept
			: m_Str{ str, N - 1 }
			, m_Hash{ ComputeHash(m_Str, 0) }
		{}

		[[nodiscard]] constexpr explicit operator std::string_view() const noexcept
		{
			return m_Str;
		}
		[[nodiscard]] constexpr explicit operator uint32() const noexcept { return m_Hash; }
		[[nodiscard]] constexpr std::string_view GetString() const noexcept { return m_Str; }
		[[nodiscard]] constexpr uint32 GetHash() const noexcept { return m_Hash; }

		[[nodiscard]] constexpr bool operator==(const HashedString other) const noexcept
		{
			return m_Hash == other.m_Hash && m_Str == other.m_Str;
		}

	private:
		static constexpr uint32 ComputeHash(std::string_view str, uint32 h) noexcept
		{
			/* Modified version of the Jenkins-One-At-A-Time hash. Not the best hash in the world,
			 * but good enough for short text strings and importantly: constexpr friendly.
			 * You can find the original algorithm here:
			 * https://en.wikipedia.org/wiki/Jenkins_hash_function#one_at_a_time
			 */
			for (const char c : str)
			{
				h += c;
				h += h << 10;
				h ^= h >> 6;
			}
			h *= 0x68f96d6d;
			h ^= h >> 7;
			h *= h << 3;
			h ^= h >> 11;
			h += h << 15;
			return h;
		}

		std::string_view m_Str;
		uint32 m_Hash = 0;
	};

	template <>
	struct Hash<HashedString>
	{
		[[nodiscard]] constexpr uint32 operator()(HashedString h) const noexcept
		{
			return h.GetHash();
		}
	};
} // namespace apollo

namespace fmt {

	/// fmt::format specialization for HashedString class
	template <>
	struct formatter<apollo::HashedString> : public formatter<fmt::basic_string_view<char>>
	{
		using Base = formatter<fmt::basic_string_view<char>>;
		fmt::appender format(const apollo::HashedString& str, format_context& ctx) const
		{
#ifndef _DOXYGEN_
			return Base::format(str.GetString(), ctx);
#endif
		}
	};
} // namespace fmt
