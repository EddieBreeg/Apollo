#pragma once

#include <PCH.hpp>
#include <string_view>

namespace brk::utf8 {
	static constexpr char32_t g_InvalidCodePoint = 0xffffffff;

	inline constexpr char* Encode(char32_t codePoint, char out_str[4]) noexcept;

	struct Decoder
	{
		constexpr Decoder(const char* str, size_t len)
			: m_Str{ str }
			, m_Len{ len }
		{}
		constexpr Decoder(std::string_view str)
			: m_Str{ str.data() }
			, m_Len{ str.size() }
		{}

		[[nodiscard]] constexpr char32_t DecodeNext() noexcept;

	private:
		const char* m_Str = nullptr;
		size_t m_Len = 0;

		static constexpr uint8 s_Accept = 0;
		static constexpr uint8 s_Reject = 12;

		// clang-format off
		static constexpr uint8 s_Utf8d[] = {
		// The first part of the table maps bytes to character classes that
		// to reduce the size of the transition table and create bitmasks.
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

		// The second part is a transition table that maps a combination
		// of a state of the automaton and a character class to a state.
		0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
		12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
		12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
		12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
		12,36,12,12,12,12,12,12,12,12,12,12, 
		};
		// clang-format on

		static uint32 Step(uint32 state, char32_t& codep, uint8 byte) noexcept;
	};

	inline constexpr char* Encode(char32_t codePoint, char out_str[4]) noexcept
	{
		if (codePoint > 0x10FFFF || !out_str)
			return nullptr;

		if (codePoint < 128u)
		{
			out_str[0] = char(codePoint);
			return ++out_str;
		}

		if (codePoint < 0x800u)
		{
			*out_str++ = char(codePoint >> 6) | 0b11000000;
			*out_str++ = char(codePoint & 0x3F) | 0b10000000;
			return out_str;
		}

		if (codePoint < 0x10000)
		{
			*out_str++ = char(codePoint >> 12) | 0b11100000;
			*out_str++ = char((codePoint >> 6) & 0x3F) | 0b10000000;
			*out_str++ = char(codePoint & 0x3F) | 0b10000000;
			return out_str;
		}

		*out_str++ = char(codePoint >> 18) | 0b11110000;
		*out_str++ = char((codePoint >> 12) & 0x3F) | 0b10000000;
		*out_str++ = char((codePoint >> 6) & 0x3F) | 0b10000000;
		*out_str++ = char(codePoint & 0x3F) | 0b10000000;

		return out_str;
	}

	inline uint32 Decoder::Step(uint32 state, char32_t& codep, uint8 byte) noexcept
	{
		// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
		// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
		const uint32 type = s_Utf8d[byte];

		codep = (state != s_Accept) ? (byte & 0x3fu) | (codep << 6) : (0xff >> type) & (byte);

		return s_Utf8d[256 + state + type];
	}

	inline constexpr char32_t Decoder::DecodeNext() noexcept
	{
		uint32 state = s_Accept;
		char32_t cp = 0;

		while (m_Len)
		{
			state = Step(state, cp, static_cast<uint8>(*m_Str++));
			--m_Len;

			if (state == s_Accept)
				return cp;
			if (state == s_Reject)
				return g_InvalidCodePoint;
		}

		return state == s_Accept ? cp : g_InvalidCodePoint;
	}
} // namespace brk::utf8
