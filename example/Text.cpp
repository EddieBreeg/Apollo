#include "Text.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/App.hpp>
#include <core/Assert.hpp>
#include <core/GameTime.hpp>
#include <core/Utf8.hpp>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <msdfgen.h>
#include <rendering/Bitmap.hpp>
#include <rendering/Renderer.hpp>

namespace brk::demo {
	FontAtlas::FontAtlas()
		: m_Texture()
		, m_PixelSize(0)
	{
		FT_Error err = FT_Init_FreeType(&m_FreetypeHandle);
		BRK_ASSERT(!err, "Failed to init Freetype: {}", FT_Error_String(err));
	}

	FontAtlas::FontAtlas(const char* path)
		: FontAtlas()
	{
		FT_Error err = FT_New_Face(m_FreetypeHandle, path, 0, &m_Face);
		DEBUG_CHECK(!err)
		{
			BRK_LOG_ERROR("Failed to load font {}: {}", path, FT_Error_String(err));
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
		m_Range = range;
		m_Range.m_First = Max(m_Range.m_First, U' ');
		m_PixelSize = size;
		std::vector<msdfgen::Shape> shapes;

		const double paddingNormalized = padding / double(size);
		rdr::txt::AtlasGenerator generator{ m_Face, maxTexWith, size, paddingNormalized };

		GameTime timer;
		// First step: load glyphs, decompose shapes, compute metrics and pack rectangles
		const auto atlasSize = generator.LoadGlyphRange(range, m_Glyphs, shapes, m_Indices);
		using Milliseconds = std::chrono::duration<double, std::milli>;
		timer.Update();
		BRK_LOG_TRACE("Glyph loading/packing took {}ms", timer.GetElapsedAs<Milliseconds>().count());

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

		timer.Reset();
		generator.Rasterize(pxRange, m_Glyphs, shapes, { buf, atlasSize.x, atlasSize.y });

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