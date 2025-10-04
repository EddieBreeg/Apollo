#pragma once

#include <PCH.hpp>

struct FT_LibraryRec_;
struct FT_FaceRec_;

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

	private:
		FT_LibraryRec_* m_FreetypeHandle = nullptr;
		FT_FaceRec_* m_Face = nullptr;
		float m_Scale = 1.0f;
	};
} // namespace brk::demo