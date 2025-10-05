#pragma once

#include <PCH.hpp>
#include <rendering/Texture.hpp>

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
	struct GlyphRange
	{
		char32_t m_First = ' ';
		char32_t m_Last = 255; // Latin-1 supplement

		[[nodiscard]] constexpr uint32 GetSize() const noexcept { return m_Last - m_First + 1; }
	};

	struct Glyph
	{
		char32_t m_Ch;
		float m_Advance;
		float2 m_Offset;
		RectU32 m_UvRect;
	};

	struct FontAtlas
	{
		FontAtlas();
		FontAtlas(const char* fontPath);
		~FontAtlas();

		FontAtlas(const FontAtlas&) = delete;
		FontAtlas(FontAtlas&&) noexcept;

		FontAtlas& operator=(const FontAtlas&) = delete;
		FontAtlas& operator=(FontAtlas&&) noexcept;

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

	private:
		bool LoadGlyph(char32_t ch, msdfgen::Shape& out_shape);
		FT_LibraryRec_* m_FreetypeHandle = nullptr;
		FT_FaceRec_* m_Face = nullptr;
		float m_Scale = 1.0f;
		GlyphRange m_Range;
		rdr::Texture2D m_Texture;
		std::vector<Glyph> m_Glyphs;
	};
} // namespace brk::demo