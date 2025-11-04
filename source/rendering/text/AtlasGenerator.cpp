#include "FontAtlas.hpp"
#include <algorithm>
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <core/ThreadPool.hpp>
#include <core/Utf8.hpp>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <latch>
#include <msdfgen.h>
#include <span>

namespace {
	struct ShapeLoader
	{
		msdfgen::Shape& m_OutShape;
		msdfgen::Contour m_Contour;
		double m_Scale = 1.0f;
		msdfgen::Point2 m_Pos;

		msdfgen::Point2 FtPoint2(const FT_Vector* pos)
		{
			return msdfgen::Point2{ m_Scale * pos->x, m_Scale * pos->y };
		}

		void FinishContour()
		{
			if (m_Contour.edges.size())
			{
				m_OutShape.addContour(std::move(m_Contour));
			}
		}

		static int StartContour(const FT_Vector* startPos, ShapeLoader* ctx)
		{
			ctx->FinishContour();
			ctx->m_Pos = ctx->FtPoint2(startPos);
			return FT_Err_Ok;
		}

		static int ToLinear(const FT_Vector* endPos, ShapeLoader* ctx)
		{
			const auto p = ctx->FtPoint2(endPos);
			ctx->m_Contour.addEdge(
				msdfgen::EdgeHolder{
					ctx->m_Pos,
					p,
				});
			ctx->m_Pos = p;
			return FT_Err_Ok;
		}

		static int ToQuadratic(const FT_Vector* cp, const FT_Vector* endPos, ShapeLoader* ctx)
		{
			const auto p = ctx->FtPoint2(endPos);
			ctx->m_Contour.addEdge(
				msdfgen::EdgeHolder{
					ctx->m_Pos,
					ctx->FtPoint2(cp),
					p,
				});
			ctx->m_Pos = p;
			return FT_Err_Ok;
		}

		static int ToCubic(
			const FT_Vector* cp0,
			const FT_Vector* cp1,
			const FT_Vector* endPos,
			ShapeLoader* ctx)
		{
			const auto p = ctx->FtPoint2(endPos);
			ctx->m_Contour.addEdge(
				msdfgen::EdgeHolder{
					ctx->m_Pos,
					ctx->FtPoint2(cp0),
					ctx->FtPoint2(cp1),
					p,
				});
			ctx->m_Pos = p;
			return FT_Err_Ok;
		}

		static inline const FT_Outline_Funcs s_OutlineFuncs{
			.move_to = (FT_Outline_MoveTo_Func)&ShapeLoader::StartContour,
			.line_to = (FT_Outline_LineToFunc)&ShapeLoader::ToLinear,
			.conic_to = (FT_Outline_ConicToFunc)&ShapeLoader::ToQuadratic,
			.cubic_to = (FT_Outline_CubicToFunc)&ShapeLoader::ToCubic,
		};
	};

	struct GridPacker
	{
		GridPacker(uint32 maxWidth)
			: m_MaxWidth(maxWidth)
		{}

		void Pack(RectU32& inout_rect)
		{
			static constexpr uint32 margin = 1;
			const uint32 width = inout_rect.GetWidth();
			const uint32 height = inout_rect.GetHeight();

			if ((m_MaxWidth - m_Pos.x) < (width + margin))
			{
				m_Pos = { 0, m_Pos.y + m_RowHeight };
				m_Height += m_RowHeight;
				m_RowHeight = 0;
			}
			inout_rect.x0 = m_Pos.x;
			inout_rect.y0 = m_Pos.y;
			inout_rect.x1 = m_Pos.x + width;
			inout_rect.y1 = m_Pos.y + height;

			m_Pos.x += width + margin;
			m_Width = apollo::Max(m_Width, m_Pos.x);
			m_RowHeight = apollo::Max(m_RowHeight, height + margin);
		}

		glm::uvec2 GetTotalSize() const noexcept { return { m_Width, m_Height + m_RowHeight }; }

	private:
		uint32 m_MaxWidth;
		uint32 m_Width = 0;
		uint32 m_Height = 0;
		uint32 m_RowHeight = 0;
		glm::uvec2 m_Pos = { 0, 0 };
	};

	struct RenderJob
	{
		using RGBA8Pixel = apollo::rdr::RGBAPixel<uint8>;

		std::latch& m_Latch;
		std::span<const apollo::rdr::txt::Glyph> m_Glyphs;
		std::span<const msdfgen::Shape> m_Shapes;
		uint32 m_Size;
		msdfgen::Range m_DistanceRange;
		std::vector<float> m_Bitmap;
		RGBA8Pixel* m_OutBuf;
		uint32 m_BufStride;

		void Render(float2 offset, const RectU32& bounds, const msdfgen::Shape& shape)
		{
			const uint32 width = bounds.GetWidth(), height = bounds.GetHeight();
			const uint32 bitmapSize = width * height * 4;
			if (!bitmapSize)
				return;

			if (m_Bitmap.size() < bitmapSize)
				m_Bitmap.resize(bitmapSize);
			const msdfgen::SDFTransformation transform{
				msdfgen::Projection{
					m_Size,
					msdfgen::Vector2{ -offset.x, -offset.y },
				},
				m_DistanceRange,
			};
			msdfgen::BitmapSection<float, 4> section{
				m_Bitmap.data(),
				static_cast<int>(width),
				static_cast<int>(height),
				msdfgen::Y_DOWNWARD,
			};
			msdfgen::generateMTSDF(section, shape, transform);

			apollo::rdr::BitmapView<RGBA8Pixel> dest{
				m_OutBuf, bounds.x0, bounds.y0, width, height, m_BufStride,
			};
			for (uint32 j = 0; j < height; ++j)
			{
				for (uint32 i = 0; i < width; ++i)
				{
					const float* inPx = section(i, j);
					dest(i, j) = RGBA8Pixel{
						uint8(255 * apollo::Clamp(inPx[0], 0.0f, 1.0f) + 0.5),
						uint8(255 * apollo::Clamp(inPx[1], 0.0f, 1.0f) + 0.5),
						uint8(255 * apollo::Clamp(inPx[2], 0.0f, 1.0f) + 0.5),
						uint8(255 * apollo::Clamp(inPx[3], 0.0f, 1.0f) + 0.5),
					};
				}
			}
		}

		void operator()()
		{
			for (uint32 i = 0; i < m_Glyphs.size(); ++i)
			{
				Render(m_Glyphs[i].m_Offset, m_Glyphs[i].m_Uv, m_Shapes[i]);
			}
			m_Latch.count_down();
		}
	};
} // namespace

namespace apollo::rdr::txt {
	AtlasGenerator::AtlasGenerator(
		FT_FaceRec_* face,
		uint32 maxWidth,
		uint32 resolution,
		double emPadding)
		: m_Face(face)
		, m_MaxWidth(maxWidth)
		, m_Res(resolution)
		, m_Padding(emPadding)
	{
		if (face && face->units_per_EM)
			m_Scale /= face->units_per_EM;

		(void)m_MaxWidth;
		(void)m_Res;
		(void)m_Padding;
	}

	bool AtlasGenerator::LoadGlyph(Glyph& inout_glyph, msdfgen::Shape& out_shape)
	{
		DEBUG_CHECK(m_Face)
		{
			APOLLO_LOG_ERROR("Called LoadGlyph on null face");
			return false;
		}
		const auto index = FT_Get_Char_Index(m_Face, inout_glyph.m_Char);
		if (!index)
			return false;
		inout_glyph.m_Index = index;

		FT_Error err = FT_Load_Glyph(m_Face, index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
		DEBUG_CHECK(!err)
		{
			char buf[5]{};
			utf8::Encode(inout_glyph.m_Char, buf);
			APOLLO_LOG_ERROR(
				"Failed to load glyph {}(U+{:08X}): {}",
				buf,
				uint32(inout_glyph.m_Char),
				FT_Error_String(err));
			return false;
		}
		inout_glyph.m_Advance = float(m_Scale * m_Face->glyph->metrics.horiAdvance);

		ShapeLoader ctx{ .m_OutShape = out_shape, .m_Scale = m_Scale };
		err = FT_Outline_Decompose(&m_Face->glyph->outline, &ShapeLoader::s_OutlineFuncs, &ctx);
		if (err)
		{
			APOLLO_LOG_ERROR("Failed to decompose glyph: {}", FT_Error_String(err));
			return false;
		}
		ctx.FinishContour();
		if (!out_shape.validate())
			return false;
		out_shape.normalize();
		out_shape.orientContours();
		msdfgen::edgeColoringSimple(out_shape, 3.0);

		return true;
	}

	glm::uvec2 AtlasGenerator::LoadGlyphRange(
		GlyphRange range,
		std::vector<Glyph>& out_glyphs,
		std::vector<msdfgen::Shape>& out_shapes,
		std::vector<uint32>& out_indices)
	{
		std::vector<uint32> tempIndices;
		range.m_First = Max(range.m_First, U' ');
		out_glyphs.reserve(range.GetSize());
		out_indices.resize(range.GetSize(), UINT32_MAX);
		out_shapes.reserve(range.GetSize());
		tempIndices.reserve(range.GetSize());

		for (char32_t ch = range.m_First; ch <= range.m_Last; ++ch)
		{
			msdfgen::Shape shape;
			Glyph glyph{
				.m_Char = ch,
				.m_Offset = float2{ 0, 0 },
				.m_Uv = {},
			};

			if (!LoadGlyph(glyph, shape))
				continue;

			const uint32 index = NumCast<uint32>(out_glyphs.size());
			out_indices[range.GetIndex(ch)] = index;
			tempIndices.emplace_back(index);

			// if shape has no contour, the character has no glyph per say, just ignore it
			if (!shape.contours.size())
			{
				out_shapes.emplace_back(std::move(shape));
				out_glyphs.emplace_back(glyph);
				continue;
			}

			const auto bounds = shape.getBounds(m_Padding);

			glyph.m_Offset = { bounds.l, bounds.b };

			glyph.m_Uv.x1 = uint32(m_Res * (bounds.r - bounds.l) + 1.5f);
			glyph.m_Uv.y1 = uint32(m_Res * (bounds.t - bounds.b) + 1.5f);
			out_glyphs.emplace_back(std::move(glyph));
			out_shapes.emplace_back(std::move(shape));
		}
		// sort glyphs by decreasing height
		std::sort(
			tempIndices.begin(),
			tempIndices.end(),
			[&](uint32 a, uint32 b)
			{
				return out_glyphs[a].m_Uv.GetHeight() > out_glyphs[b].m_Uv.GetHeight();
			});
		GridPacker packer{ m_MaxWidth };
		for (uint32 index : tempIndices)
		{
			if (out_shapes[index].contours.empty())
				continue;

			Glyph& glyph = out_glyphs[index];
			packer.Pack(glyph.m_Uv);
		}

		return packer.GetTotalSize();
	}

	void AtlasGenerator::Rasterize(
		double pxRange,
		const std::vector<Glyph>& glyphs,
		const std::vector<msdfgen::Shape>& shapes,
		rdr::BitmapView<rdr::RGBAPixel<uint8>> out_bitmap,
		mt::ThreadPool& threadPool)
	{
		const msdfgen::Range distMapping{ pxRange / m_Res };

		const uint32 numThreads = threadPool.GetThreadCount();
		std::latch latch{ numThreads };
		const uint32 numGlyphs = NumCast<uint32>(glyphs.size());
		APOLLO_ASSERT(
			shapes.size() == numGlyphs,
			"Can't rasterize glyphs: glyphs.size() is {} but shapes.size() is {}",
			numGlyphs,
			shapes.size());

		APOLLO_LOG_TRACE("Running {} threads", numThreads);
		const uint32 glyphsPerThread = NumCast<uint32>(glyphs.size()) / numThreads;
		uint32 jobStartIndex = 0;

		for (uint32 i = 0; i < numThreads; ++i)
		{
			const Glyph* gPtr = glyphs.data() + jobStartIndex;
			const msdfgen::Shape* sPtr = shapes.data() + jobStartIndex;

			const uint32 jobSize = glyphsPerThread + (i < (numGlyphs % numThreads));
			threadPool.Enqueue(
				RenderJob{
					.m_Latch = latch,
					.m_Glyphs = std::span{ gPtr, jobSize },
					.m_Shapes = std::span{ sPtr, jobSize },
					.m_Size = m_Res,
					.m_DistanceRange = distMapping,
					.m_Bitmap = std::vector<float>(m_Res * m_Res * 3),
					.m_OutBuf = out_bitmap.GetData(),
					.m_BufStride = out_bitmap.GetStride(),
				});
			jobStartIndex += jobSize;
		}
		latch.wait();
	}
} // namespace apollo::rdr::txt