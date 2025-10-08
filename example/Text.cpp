#include "Text.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/App.hpp>
#include <core/Assert.hpp>
#include <core/GameTime.hpp>
#include <core/Utf8.hpp>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <latch>
#include <msdfgen.h>
#include <rendering/Bitmap.hpp>
#include <rendering/Renderer.hpp>
#include <span>

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
			m_Width = brk::Max(m_Width, m_Pos.x);
			m_RowHeight = brk::Max(m_RowHeight, height + margin);
		}

		glm::uvec2 GetTotalSize() const noexcept { return { m_Width, m_Height + m_RowHeight }; }

	private:
		uint32 m_MaxWidth;
		uint32 m_Width = 0;
		uint32 m_Height = 0;
		uint32 m_RowHeight = 0;
		glm::uvec2 m_Pos = { 0, 0 };
	};

	void CopyBitmap(
		const msdfgen::BitmapSection<float, 4>& src,
		brk::rdr::BitmapView<brk::rdr::Pixel<uint8, 4>> dest)
	{
		using Pixel = brk::rdr::Pixel<uint8, 4>;
		const uint32 w = dest.GetWidth();
		const uint32 h = dest.GetHeight();
		for (uint32 j = 0; j < h; ++j)
		{
			for (uint32 i = 0; i < w; ++i)
			{
				const float* inPx = src(i, j);
				dest(i, j) = Pixel{
					uint8(255 * brk::Clamp(inPx[0], 0.0f, 1.0f) + 0.5),
					uint8(255 * brk::Clamp(inPx[1], 0.0f, 1.0f) + 0.5),
					uint8(255 * brk::Clamp(inPx[2], 0.0f, 1.0f) + 0.5),
					uint8(255 * brk::Clamp(inPx[3], 0.0f, 1.0f) + 0.5),
				};
			}
		}
	}

	struct Rasterizer
	{
		using RGBA8Pixel = brk::rdr::RGBAPixel<uint8>;

		uint32 m_Size;
		msdfgen::Range m_DistanceRange;
		std::vector<float> m_Bitmap;
		RGBA8Pixel* m_OutBuf;
		uint32 m_BufStride;

		void operator()(float2 offset, const RectU32& bounds, const msdfgen::Shape& shape)
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
			CopyBitmap(
				section,
				brk::rdr::BitmapView<RGBA8Pixel>{
					m_OutBuf,
					bounds.x0,
					bounds.y0,
					width,
					height,
					m_BufStride,
				});
		}
	};

	struct RenderJob
	{
		std::latch& m_Latch;
		std::span<const brk::demo::Glyph> m_Glyphs;
		std::span<const msdfgen::Shape> m_Shapes;
		Rasterizer m_Rasterizer;
		std::chrono::duration<double, std::milli>& m_OutTimer;

		void operator()()
		{
			auto t = std::chrono::steady_clock::now();
			for (uint32 i = 0; i < m_Glyphs.size(); ++i)
			{
				m_Rasterizer(m_Glyphs[i].m_Offset, m_Glyphs[i].m_UvRect, m_Shapes[i]);
			}
			m_OutTimer = std::chrono::steady_clock::now() - t;
			m_Latch.count_down();
		}
	};

} // namespace

namespace brk::demo {
	FontAtlas::FontAtlas()
		: m_Texture()
		, m_PixelSize(0)
	{
		FT_Error err = FT_Init_FreeType(&m_FreetypeHandle);
		BRK_ASSERT(!err, "Failed to init Freetype: {}", GetFreetypeErrMsg(err));
	}

	FontAtlas::FontAtlas(const char* path)
		: FontAtlas()
	{
		FT_Error err = FT_New_Face(m_FreetypeHandle, path, 0, &m_Face);
		DEBUG_CHECK(!err)
		{
			BRK_LOG_ERROR("Failed to load font {}: {}", path, GetFreetypeErrMsg(err));
			return;
		}
	}

	FontAtlas::FontAtlas(FontAtlas&& other) noexcept
		: m_FreetypeHandle(other.m_FreetypeHandle)
		, m_Face(other.m_Face)
		, m_Range(other.m_Range)
		, m_Texture(std::move(other.m_Texture))
		, m_Glyphs(std::move(other.m_Glyphs))
		, m_Indices(std::move(other.m_Indices))
		, m_PixelSize(other.m_PixelSize)
	{
		other.m_FreetypeHandle = nullptr;
		other.m_Face = nullptr;
		other.m_Range = GlyphRange{ 0, 0 };
		other.m_PixelSize = 0;
	}

	FontAtlas& FontAtlas::operator=(FontAtlas&& other) noexcept
	{
		Swap(other);
		return *this;
	}

	void FontAtlas::Swap(FontAtlas& other) noexcept
	{
		std::swap(m_Face, other.m_Face);
		std::swap(m_FreetypeHandle, other.m_FreetypeHandle);
		std::swap(m_PixelSize, other.m_PixelSize);
		std::swap(m_Range, other.m_Range);
		m_Texture.Swap(other.GetTexture());
		m_Glyphs.swap(other.m_Glyphs);
		m_Indices.swap(other.m_Indices);
	}

	bool FontAtlas::LoadGlyph(char32_t ch, msdfgen::Shape& out_shape, float scale)
	{
		DEBUG_CHECK(m_Face)
		{
			BRK_LOG_ERROR("Called LoadGlyph on null face");
			return false;
		}
		const auto index = FT_Get_Char_Index(m_Face, ch);
		if (!index)
			return false;

		FT_Error err = FT_Load_Glyph(m_Face, index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
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

		GlyphContext ctx{ .m_OutShape = out_shape, .m_Scale = scale };
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
		out_shape.orientContours();
		msdfgen::edgeColoringSimple(out_shape, 3.0);

		return true;
	}

	FontAtlas::~FontAtlas()
	{
		if (m_Face)
			FT_Done_Face(m_Face);
		FT_Done_FreeType(m_FreetypeHandle);
	}

	rdr::Texture2D& FontAtlas::LoadRange(
		SDL_GPUCopyPass* copyPass,
		GlyphRange range,
		uint32 size,
		float pxRange,
		uint32 maxTexWith,
		uint32 padding)
	{
		range.m_First = Max(range.m_First, U' ');
		m_Range = range;
		m_PixelSize = size;
		m_Glyphs.reserve(range.GetSize());
		m_Indices.resize(range.GetSize(), UINT32_MAX);
		std::vector<msdfgen::Shape> shapes;
		shapes.reserve(range.GetSize());
		float scale = m_Face->units_per_EM ? (1.0f / m_Face->units_per_EM) : 1.0f;

		GridPacker packer{ maxTexWith };

		const double paddingNormalized = padding / double(size);

		GameTime timer;
		// First step: load glyphs, decompose shapes, compute metrics and pack rectangles
		for (char32_t ch = range.m_First; ch <= range.m_Last; ++ch)
		{
			msdfgen::Shape shape;

			if (!LoadGlyph(ch, shape, scale))
				continue;

			m_Indices[range.GetIndex(ch)] = (uint32)m_Glyphs.size();

			Glyph glyph{
				.m_Ch = ch,
				.m_Advance = m_Face->glyph->metrics.horiAdvance * scale,
				.m_Offset = float2{ 0, 0 },
				.m_UvRect = {},
			};

			// if shape has no contour, the character has no glyph per say, just ignore it
			if (!shape.contours.size())
			{
				shapes.emplace_back(std::move(shape));
				m_Glyphs.emplace_back(glyph);
				continue;
			}

			DEBUG_CHECK(shape.validate())
			{
				BRK_LOG_ERROR("Failed to validate shape for glyph U+{:08X}", uint32(ch));
				continue;
			}
			const auto bounds = shape.getBounds(paddingNormalized);

			glyph.m_Offset = { bounds.l, bounds.b };

			glyph.m_UvRect.x1 = uint32(size * (bounds.r - bounds.l) + 1.5f);
			glyph.m_UvRect.y1 = uint32(size * (bounds.t - bounds.b) + 1.5f);
			packer.Pack(glyph.m_UvRect);
			m_Glyphs.emplace_back(std::move(glyph));
			shapes.emplace_back(std::move(shape));
		}
		using Milliseconds = std::chrono::duration<double, std::milli>;
		timer.Update();
		BRK_LOG_TRACE("Glyph loading/packing took {}ms", timer.GetElapsedAs<Milliseconds>().count());

		const auto atlasSize = packer.GetTotalSize();
		const uint32 pixelCount = atlasSize.x * atlasSize.y;

		m_Texture = rdr::Texture2D{ rdr::TextureSettings{
			atlasSize.x,
			atlasSize.y,
			rdr::EPixelFormat::RGBA8_UNorm,
			rdr::ETextureUsageFlags::Sampled,
		} };
		using RGBA8Pixel = rdr::RGBAPixel<uint8>;

		const SDL_GPUTransferBufferCreateInfo tBufferInfo{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = uint32(sizeof(rdr::RGBAPixel<RGBA8Pixel>) * pixelCount),
		};
		SDL_GPUDevice* device = rdr::Renderer::GetInstance()->GetDevice().GetHandle();

		SDL_GPUTransferBuffer* transferBuf = SDL_CreateGPUTransferBuffer(device, &tBufferInfo);
		RGBA8Pixel* buf = (RGBA8Pixel*)SDL_MapGPUTransferBuffer(device, transferBuf, true);

		const msdfgen::Range distRange{ pxRange / size };

		mt::ThreadPool& threadPool = App::GetInstance()->GetThreadPool();
		const uint32 numThreads = threadPool.GetThreadCount();
		Milliseconds* threadTimers = Alloca(Milliseconds, numThreads);
		std::latch latch{ numThreads };

		BRK_LOG_TRACE("Running {} threads", numThreads);
		const uint32 glyphsPerThread = m_Glyphs.size() / numThreads;
		uint32 jobStartIndex = 0;

		timer.Reset();
		for (uint32 i = 0; i < numThreads; ++i)
		{
			const Glyph* gPtr = m_Glyphs.data() + jobStartIndex;
			const msdfgen::Shape* sPtr = shapes.data() + jobStartIndex;

			const uint32 jobSize = glyphsPerThread + (i < (m_Glyphs.size() % numThreads));
			threadPool.Enqueue(
				RenderJob{
					.m_Latch = latch,
					.m_Glyphs = std::span{gPtr, jobSize},
					.m_Shapes = std::span{sPtr, jobSize},
					.m_Rasterizer ={
						.m_Size = size,
						.m_DistanceRange = distRange,
						.m_Bitmap = std::vector<float>(size * size * 3),
						.m_OutBuf = buf,
						.m_BufStride = atlasSize.x,
					},
					.m_OutTimer = *(new(threadTimers + i)Milliseconds{}),
				}
			);
			jobStartIndex += jobSize;
		}
		latch.wait();

		for (uint32 i = 0; i < numThreads; ++i)
		{
			BRK_LOG_TRACE("Thread {} took {}ms", i, threadTimers[i].count());
		}
		timer.Update();

		BRK_LOG_TRACE("Glyph MSDF rendering took {}ms", timer.GetElapsedAs<Milliseconds>().count());

		const SDL_GPUTextureTransferInfo srcLoc{
			.transfer_buffer = transferBuf,
			.pixels_per_row = atlasSize.x,
			.rows_per_layer = atlasSize.y,
		};
		const SDL_GPUTextureRegion destRegion{
			.texture = m_Texture.GetHandle(),
			.w = atlasSize.x,
			.h = atlasSize.y,
			.d = 1,
		};
		SDL_UploadToGPUTexture(copyPass, &srcLoc, &destRegion, true);

		SDL_ReleaseGPUTransferBuffer(device, transferBuf);
		BRK_LOG_TRACE("Atlas texture size: ({}, {})", atlasSize.x, atlasSize.y);
		return m_Texture;
	}

	const Glyph* FontAtlas::GetGlyph(char32_t c, char32_t fallback) const noexcept
	{
		if (const Glyph* g = FindGlyph(c))
			return g;
		return FindGlyph(fallback);
	}

	float2 FontAtlas::GetKerning(char32_t left, char32_t right) const
	{
		if (!FT_HAS_KERNING(m_Face))
			return { 0, 0 };

		const uint32 leftIndex = FT_Get_Char_Index(m_Face, left);
		const uint32 rightIndex = FT_Get_Char_Index(m_Face, right);

		const float scale = m_Face->units_per_EM ? (1.0F / m_Face->units_per_EM) : 1.0f;
		FT_Vector vec{ 0, 0 };
		const FT_Error
			err = FT_Get_Kerning(m_Face, leftIndex, rightIndex, FT_KERNING_UNSCALED, &vec);
		(void)err;
		return {
			scale * vec.x,
			scale * vec.y,
		};
	}

	const Glyph* FontAtlas::FindGlyph(char32_t c) const noexcept
	{
		const uint32 i = m_Range.GetIndex(c);
		if (i > m_Indices.size())
			return nullptr;

		if (const uint32 j = m_Indices[i]; j < m_Glyphs.size())
			return m_Glyphs.data() + j;
		return nullptr;
	}
} // namespace brk::demo