#include "FontAtlas.hpp"
#include "Style.hpp"
#include <core/Utf8.hpp>
#include <freetype/freetype.h>

namespace apollo::rdr::txt {
	FontAtlas::FontAtlas(
		FT_FaceRec_* face,
		uint32 size,
		GlyphRange range,
		std::vector<Glyph> glyphs,
		std::vector<uint32> indices,
		Texture2D texture)
		: m_FaceHandle(face)
		, m_Range(range)
		, m_Texture(std::move(texture))
		, m_Glyphs(std::move(glyphs))
		, m_Indices(std::move(indices))
		, m_PixelSize(size)
	{}

	FontAtlas::~FontAtlas()
	{
		if (m_FaceHandle)
			FT_Done_Face(m_FaceHandle);
	}

	const Glyph* FontAtlas::FindGlyph(char32_t ch) const noexcept
	{
		const uint32 i = m_Range.GetIndex(ch);
		if (i > m_Indices.size())
			return nullptr;

		if (const uint32 j = m_Indices[i]; j < m_Glyphs.size())
			return m_Glyphs.data() + j;
		return nullptr;
	}

	float2 FontAtlas::GetKerning(const Glyph& left, const Glyph& right) const noexcept
	{
		if (!FT_HAS_KERNING(m_FaceHandle))
			return { 0, 0 };

		const float scale = m_FaceHandle->units_per_EM ? (1.0F / m_FaceHandle->units_per_EM) : 1.0f;
		FT_Vector vec{ 0, 0 };
		const FT_Error err = FT_Get_Kerning(
			m_FaceHandle,
			left.m_Index,
			right.m_Index,
			FT_KERNING_UNSCALED,
			&vec);
		(void)err;
		return {
			scale * vec.x,
			scale * vec.y,
		};
	}

	float2 FontAtlas::MeasureText(std::string_view txt, const TextStyle& style, char32_t fallback)
		const noexcept
	{
		RectF bounds{ 0, 0, 0, 0 };
		const float scale = 1.0f / m_PixelSize;
		const Glyph* prev = nullptr;
		utf8::Decoder decoder{ txt };
		float2 pos = {};

		while (char32_t cp = decoder.DecodeNext())
		{
			if (cp == utf8::g_InvalidCodePoint)
				cp = fallback;

			const Glyph* glyph = GetGlyph(cp, fallback);
			if (!glyph) [[unlikely]]
				continue;
			const uint32 width = glyph->m_Uv.GetWidth(), height = glyph->m_Uv.GetHeight();
			if (cp == '\n')
			{
				pos = float2{ 0, pos.y - style.m_Size * style.m_LineSpacing };
				continue;
			}
			else if (!(width && height))
			{
				pos.x += style.m_Size * style.m_Tracking * glyph->m_Advance;
				continue;
			}
			if (prev)
				pos += style.m_Size * style.m_Kerning * GetKerning(*prev, *glyph);

			const float2 glyphSize{
				scale * width,
				scale * height,
			};
			const float4 rect{
				pos + style.m_Size * glyph->m_Offset,
				style.m_Size * glyphSize,
			};

			bounds += RectF{
				rect.x,
				rect.y,
				rect.x + rect.z,
				rect.y + rect.w,
			};
			pos.x += style.m_Size * style.m_Tracking * glyph->m_Advance;
			prev = glyph;
		}

		return float2{
			bounds.x1 - bounds.x0,
			bounds.y1 - bounds.y0,
		};
	}
} // namespace apollo::rdr::txt