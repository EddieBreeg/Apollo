#pragma once

#include <PCH.hpp>

#include "Hash.hpp"
#include "JsonFwd.hpp"
#include <string_view>

namespace brk {
	/**
	 * Unique lexographically sortable identifier. The spec for this format can
	 * be found here: https://github.com/ulid/spec
	 */
	class BRK_API ULID
	{
	public:
		/**
		 * Creates a null identifier (all 0s)
		 */
		constexpr ULID() noexcept = default;
		constexpr ULID(const ULID&) noexcept = default;
		constexpr ~ULID() = default;
		constexpr ULID& operator=(const ULID&) noexcept = default;

		/**
		 * Constructs a ULID from a timestamp and random values
		 * \note Only the 48 rightmost bit of timestamp will be used, the 16 upper bits will be
		 * discarded
		 */
		constexpr ULID(const uint64 timestamp, const uint16 rand1, const uint64 rand2) noexcept;

		/**
		 * Generates a new identifier, with a 48-bit unix timestamp (in milliseconds) and
		 * 80 bits of pseudo-random data
		 */
		[[nodiscard]] static ULID Generate();

		/**
		 * Converts the ULID object into a base 32 string
		 * \tparam N: Size of the output buffer
		 * \tparam Offset: Start position of the ULID in the output buffer
		 * \param out_buf: array of characters where to write the ULID
		 * \returns A pointer to the character right past the end of the ULID in the output buffer
		 */
		template <uint32 N, uint32 Offset = 0>
		constexpr char* ToChars(char (&out_buf)[N]) const noexcept;

		/**
		 * Attempts to create a ULID object from a base 32 string. If the string doesn't
		 * represent a valid ULID object, a null id is returned instead
		 */
		[[nodiscard]] static constexpr ULID FromString(const std::string_view str) noexcept;

		/**
		 * Tests whether this id is non-null
		 */
		[[nodiscard]] constexpr operator bool() const noexcept { return m_Left || m_Right; }

		[[nodiscard]] constexpr bool operator==(const ULID other) const noexcept;
		[[nodiscard]] constexpr bool operator!=(const ULID other) const noexcept;

		bool FromJson(const nlohmann::json& json) noexcept;
		void ToJson(nlohmann::json& out_json) const;

	private:
		friend struct Hash<ULID>;
		uint64 m_Left = 0, m_Right = 0;
	};

	template <>
	struct Hash<ULID>
	{
		[[nodiscard]] constexpr uint64 operator()(const ULID& id) const noexcept
		{
			return HashCombine(id.m_Left, id.m_Right);
		}
	};

	inline constexpr brk::ULID::ULID(
		const uint64 timestamp,
		const uint16 rand1,
		const uint64 rand2) noexcept
		: m_Left{ (timestamp << 16) | rand1 }
		, m_Right{ rand2 }
	{}

	template <uint32 N, uint32 Offset>
	inline constexpr char* brk::ULID::ToChars(char (&out_buf)[N]) const noexcept
	{
		static_assert(N >= (26 + Offset), "Buffer is too small");
		constexpr char alphabet[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

		out_buf[0 + Offset] = alphabet[(m_Left >> 61) & 31];
		out_buf[1 + Offset] = alphabet[(m_Left >> 56) & 31];
		out_buf[2 + Offset] = alphabet[(m_Left >> 51) & 31];
		out_buf[3 + Offset] = alphabet[(m_Left >> 46) & 31];
		out_buf[4 + Offset] = alphabet[(m_Left >> 41) & 31];
		out_buf[5 + Offset] = alphabet[(m_Left >> 36) & 31];
		out_buf[6 + Offset] = alphabet[(m_Left >> 31) & 31];
		out_buf[7 + Offset] = alphabet[(m_Left >> 26) & 31];
		out_buf[8 + Offset] = alphabet[(m_Left >> 21) & 31];
		out_buf[9 + Offset] = alphabet[(m_Left >> 16) & 31];
		out_buf[10 + Offset] = alphabet[(m_Left >> 11) & 31];
		out_buf[11 + Offset] = alphabet[(m_Left >> 6) & 31];
		out_buf[12 + Offset] = alphabet[(m_Left >> 1) & 31];
		out_buf[13 + Offset] = alphabet[((m_Left << 4) | (m_Right >> 60)) & 31];
		out_buf[14 + Offset] = alphabet[(m_Right >> 55) & 31];
		out_buf[15 + Offset] = alphabet[(m_Right >> 50) & 31];
		out_buf[16 + Offset] = alphabet[(m_Right >> 45) & 31];
		out_buf[17 + Offset] = alphabet[(m_Right >> 40) & 31];
		out_buf[18 + Offset] = alphabet[(m_Right >> 35) & 31];
		out_buf[19 + Offset] = alphabet[(m_Right >> 30) & 31];
		out_buf[20 + Offset] = alphabet[(m_Right >> 25) & 31];
		out_buf[21 + Offset] = alphabet[(m_Right >> 20) & 31];
		out_buf[22 + Offset] = alphabet[(m_Right >> 15) & 31];
		out_buf[23 + Offset] = alphabet[(m_Right >> 10) & 31];
		out_buf[24 + Offset] = alphabet[(m_Right >> 5) & 31];
		out_buf[25 + Offset] = alphabet[m_Right & 31];

		return out_buf + 26;
	}

	inline constexpr brk::ULID brk::ULID::FromString(const std::string_view str) noexcept
	{
		if (str.length() < 26)
			return {};
		constexpr uint8 map[] = {
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			0,	 1,	  2,   3,	4,	 5,	  6,   7,	8,	 9,	  255, 255, 255, 255, 255, 255,
			255, 10,  11,  12,	13,	 14,  15,  16,	17,	 255, 18,  19,	255, 20,  21,  255,
			22,	 23,  24,  25,	26,	 255, 27,  28,	29,	 30,  31,  255, 255, 255, 255, 255,
			255, 10,  11,  12,	13,	 14,  15,  16,	17,	 255, 18,  19,	255, 20,  21,  255,
			22,	 23,  24,  25,	26,	 255, 27,  28,	29,	 30,  31,  255, 255, 255, 255, 255,
		};
		brk::ULID res;

		uint8 n = 0;
		for (uint8 i = 0; i < 13; ++i)
		{
			n = map[(int32)str[i]];
			if (n == 0xff)
				return {};
			res.m_Left = (res.m_Left << 5) | n;
		}
		n = map[int32(str[13])];
		res.m_Left = (res.m_Left << 1) | (n >> 4);
		res.m_Right |= n & 0xff;

		for (uint8 i = 14; i < 26; i++)
		{
			n = map[(int32)str[i]];
			if (n == 0xff)
				return {};
			res.m_Right = (res.m_Right << 5) | n;
		}

		return res;
	}

	inline constexpr bool brk::ULID::operator==(const ULID other) const noexcept
	{
		return m_Left == other.m_Left && m_Right == other.m_Right;
	}

	inline constexpr bool brk::ULID::operator!=(const ULID other) const noexcept
	{
		return m_Left != other.m_Left || m_Right != other.m_Right;
	}

	namespace ulid_literal {
		consteval ULID operator""_ulid(const char* str, size_t len)
		{
			return ULID::FromString({ str, len });
		}
	} // namespace ulid_literal
	using namespace ulid_literal;
} // namespace brk