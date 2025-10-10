#pragma once

#include <PCH.hpp>

namespace brk::rdr::txt {
	struct GlyphRange
	{
		char32_t m_First = ' ';
		char32_t m_Last = 255; // Latin-1 supplement

		[[nodiscard]] constexpr uint32 GetSize() const noexcept { return m_Last - m_First + 1; }
		[[nodiscard]] constexpr uint32 GetIndex(char32_t c) const noexcept
		{
			return (c >= m_First && c <= m_Last) ? (c - m_First) : UINT32_MAX;
		}
	};

	struct Glyph
	{
		char32_t m_Char;
		uint32 m_Index;
		float m_Advance;
		float2 m_Offset;
		RectU32 m_Uv;
	};
}