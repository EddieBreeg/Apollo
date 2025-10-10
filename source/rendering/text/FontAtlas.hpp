#pragma once

#include <PCH.hpp>

#include "Glyph.hpp"
#include <rendering/Bitmap.hpp>
#include <vector>

struct FT_FaceRec_;

namespace msdfgen {
	class Shape;
} // namespace msdfgen

namespace brk::rdr::txt {
	class AtlasGenerator
	{
	public:
		BRK_API AtlasGenerator(
			FT_FaceRec_* face,
			uint32 maxWidth = 1024,
			uint32 resolution = 64,
			double emPadding = 1.0 / 64);

		BRK_API bool LoadGlyph(Glyph& inout_glyph, msdfgen::Shape& out_shape);

		/*
		 * Loads a range of glyphs, decomposes their shapes, packs them and returns the total atlas
		 * size
		 */
		BRK_API glm::uvec2 LoadGlyphRange(
			GlyphRange range,
			std::vector<Glyph>& out_glyphs,
			std::vector<msdfgen::Shape>& out_shapes,
			std::vector<uint32>& out_indices);

		/*
		 * Generates MSDF data. This assumes the glyphs and shapes have been previously loaded using
		 * LoadGlyphRange, and that the output bitmap is big enough
		 */
		BRK_API void Rasterize(
			double pxRange,
			const std::vector<Glyph>& glyphs,
			const std::vector<msdfgen::Shape>& shapes,
			rdr::BitmapView<rdr::RGBAPixel<uint8>> out_bitmap);

	private:
		FT_FaceRec_* m_Face;
		uint32 m_MaxWidth;
		uint32 m_Res;
		double m_Padding;
		double m_Scale = 1.0;
	};
} // namespace brk::rdr::txt