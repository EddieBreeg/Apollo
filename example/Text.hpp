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

	struct FreetypeContext
	{
		FreetypeContext();
		FreetypeContext(const char* fontPath);
		~FreetypeContext();

		FreetypeContext(const FreetypeContext&) = delete;
		FreetypeContext(FreetypeContext&&) noexcept;

		FreetypeContext& operator=(const FreetypeContext&) = delete;
		FreetypeContext& operator=(FreetypeContext&&) noexcept;

		void Swap(FreetypeContext& other) noexcept;

		bool LoadGlyph(char32_t ch, msdfgen::Shape& out_shape);
		rdr::Texture2D LoadRange(
			SDL_GPUCopyPass* copyPass,
			GlyphRange range,
			float size,
			float pxRange,
			uint32 padding = 1);

	private:
		FT_LibraryRec_* m_FreetypeHandle = nullptr;
		FT_FaceRec_* m_Face = nullptr;
		float m_Scale = 1.0f;
	};
} // namespace brk::demo