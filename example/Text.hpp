#pragma once

#include <PCH.hpp>
#include <rendering/Texture.hpp>
#include <rendering/text/FontAtlas.hpp>

struct FT_FaceRec_;
struct FT_LibraryRec_;
struct SDL_GPUCopyPass;

namespace msdfgen {
	class Shape;
}

namespace brk::rdr {
	template <class T, uint32 N>
	struct Pixel;

	template <class Pixel>
	class BitmapView;
} // namespace brk::rdr

namespace brk::demo {
	using rdr::txt::Glyph;
	using rdr::txt::GlyphRange;

	struct FontAtlas
	{
		FontAtlas();
		FontAtlas(const char* fontPath);
		~FontAtlas();

		FontAtlas(const FontAtlas&) = delete;
		FontAtlas(FontAtlas&&) noexcept;

		FontAtlas& operator=(const FontAtlas&) = delete;
		FontAtlas& operator=(FontAtlas&&) noexcept;

		float2 GetKerning(char32_t left, char32_t right) const;

		void Swap(FontAtlas& other) noexcept;

		rdr::Texture2D& LoadRange(
			SDL_GPUCopyPass* copyPass,
			GlyphRange range,
			uint32 size,
			float pxRange,
			uint32 maxTexWith = 512,
			uint32 padding = 1);

		[[nodiscard]] rdr::Texture2D& GetTexture() noexcept { return m_Texture; }
		[[nodiscard]] const rdr::Texture2D& GetTexture() const noexcept { return m_Texture; }
		[[nodiscard]] const Glyph* GetGlyph(char32_t c, char32_t fallback = ' ') const noexcept;
		[[nodiscard]] uint32 GetPixelSize() const noexcept { return m_PixelSize; }

	private:
		const Glyph* FindGlyph(char32_t ch) const noexcept;

		FT_LibraryRec_* m_FreetypeHandle = nullptr;
		FT_FaceRec_* m_Face = nullptr;
		GlyphRange m_Range;
		rdr::Texture2D m_Texture;
		std::vector<Glyph> m_Glyphs;
		std::vector<uint32> m_Indices;
		uint32 m_PixelSize;
	};
} // namespace brk::demo