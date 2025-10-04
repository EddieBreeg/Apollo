#include "Text.hpp"
#include <core/Assert.hpp>
#include <core/Utf8.hpp>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <msdfgen.h>
#include <rendering/Bitmap.hpp>

namespace {
#undef FTERRORS_H_
#define FT_ERROR_START_LIST                                                                        \
	const char* GetFreetypeErrMsg(int code)                                                        \
	{                                                                                              \
		switch (code)                                                                              \
		{
#define FT_ERRORDEF(e, v, s)                                                                       \
	case e: return s;
#define FT_ERROR_END_LIST                                                                          \
	default: return "Unknown";                                                                     \
		}                                                                                          \
		}

#include "freetype/fterrors.h"

	struct GlyphContext
	{
		msdfgen::Shape& m_OutShape;
		msdfgen::Contour m_Contour;
		float m_Scale = 1.0f;
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

		static int StartContour(const FT_Vector* startPos, GlyphContext* ctx)
		{
			ctx->FinishContour();
			ctx->m_Pos = ctx->FtPoint2(startPos);
			return FT_Err_Ok;
		}

		static int ToLinear(const FT_Vector* endPos, GlyphContext* ctx)
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

		static int ToQuadratic(const FT_Vector* cp, const FT_Vector* endPos, GlyphContext* ctx)
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
			GlyphContext* ctx)
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
			.move_to = (FT_Outline_MoveTo_Func)&GlyphContext::StartContour,
			.line_to = (FT_Outline_LineToFunc)&GlyphContext::ToLinear,
			.conic_to = (FT_Outline_ConicToFunc)&GlyphContext::ToQuadratic,
			.cubic_to = (FT_Outline_CubicToFunc)&GlyphContext::ToCubic,
		};
	};
} // namespace

namespace brk::demo {
	FreetypeContext::FreetypeContext()
	{
		FT_Error err = FT_Init_FreeType(&m_FreetypeHandle);
		BRK_ASSERT(!err, "Failed to init Freetype: {}", GetFreetypeErrMsg(err));
	}

	FreetypeContext::FreetypeContext(const char* path)
		: FreetypeContext()
	{
		FT_Error err = FT_New_Face(m_FreetypeHandle, path, 0, &m_Face);
		DEBUG_CHECK(!err)
		{
			BRK_LOG_ERROR("Failed to load font {}: {}", path, GetFreetypeErrMsg(err));
			return;
		}
		m_Scale = m_Face->units_per_EM ? (1.0f / m_Face->units_per_EM) : 1.0f;
	}

	FreetypeContext::FreetypeContext(FreetypeContext&& other) noexcept
		: m_FreetypeHandle(other.m_FreetypeHandle)
		, m_Face(other.m_Face)
		, m_Scale(other.m_Scale)
	{
		other.m_FreetypeHandle = nullptr;
		other.m_Face = nullptr;
	}

	FreetypeContext& FreetypeContext::operator=(FreetypeContext&& other) noexcept
	{
		Swap(other);
		return *this;
	}

	void FreetypeContext::Swap(FreetypeContext& other) noexcept
	{
		std::swap(m_Face, other.m_Face);
		std::swap(m_FreetypeHandle, other.m_FreetypeHandle);
		std::swap(m_Scale, other.m_Scale);
	}

	bool FreetypeContext::LoadGlyph(char32_t ch, msdfgen::Shape& out_shape)
	{
		DEBUG_CHECK(m_Face)
		{
			BRK_LOG_ERROR("Called LoadGlyph on null face");
			return false;
		}
		const auto index = FT_Get_Char_Index(m_Face, ch);
		if (!index)
			return false;

		FT_Error err = FT_Load_Glyph(m_Face, index, FT_LOAD_NO_BITMAP);
		DEBUG_CHECK(!err)
		{
			char buf[5]{};
			utf8::Encode(ch, buf);
			BRK_LOG_ERROR(
				"Failed to load glyph {}(U+{:08X}): {}",
				buf,
				uint32(ch),
				GetFreetypeErrMsg(err));
			return false;
		}

		GlyphContext ctx{ .m_OutShape = out_shape, .m_Scale = m_Scale };
		err = FT_Outline_Decompose(&m_Face->glyph->outline, &GlyphContext::s_OutlineFuncs, &ctx);
		if (err)
		{
			BRK_LOG_ERROR("Failed to decompose glyph: {}", GetFreetypeErrMsg(err));
			return false;
		}
		ctx.FinishContour();
		if (!out_shape.validate())
			return false;
		out_shape.normalize();
		msdfgen::edgeColoringSimple(out_shape, 3.0);

		return true;
	}

	FreetypeContext::~FreetypeContext()
	{
		if (m_Face)
			FT_Done_Face(m_Face);
		FT_Done_FreeType(m_FreetypeHandle);
	}
} // namespace brk::demo