#include "FontAtlas.hpp"
#include <freetype/freetype.h>

namespace brk::rdr::txt {
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
} // namespace brk::rdr::txt