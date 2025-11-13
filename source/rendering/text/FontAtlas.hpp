#pragma once

#include <PCH.hpp>

#include "Glyph.hpp"
#include <asset/Asset.hpp>
#include <rendering/Bitmap.hpp>
#include <rendering/Texture.hpp>
#include <vector>

struct FT_FaceRec_;

namespace msdfgen {
	class Shape;
} // namespace msdfgen

namespace apollo::editor {
	template <class>
	struct AssetHelper;
}

namespace apollo::mt {
	class ThreadPool;
}

namespace apollo::rdr::txt {
	struct TextStyle;

	class AtlasGenerator
	{
	public:
		APOLLO_API AtlasGenerator(
			FT_FaceRec_* face,
			uint32 maxWidth = 1024,
			uint32 resolution = 64,
			double emPadding = 1.0 / 64);

		APOLLO_API bool LoadGlyph(Glyph& inout_glyph, msdfgen::Shape& out_shape);

		/*
		 * Loads a range of glyphs, decomposes their shapes, packs them and returns the total atlas
		 * size
		 */
		APOLLO_API glm::uvec2 LoadGlyphRange(
			GlyphRange range,
			std::vector<Glyph>& out_glyphs,
			std::vector<msdfgen::Shape>& out_shapes,
			std::vector<uint32>& out_indices);

		/*
		 * Generates MSDF data. This assumes the glyphs and shapes have been previously loaded using
		 * LoadGlyphRange, and that the output bitmap is big enough
		 */
		APOLLO_API void Rasterize(
			double pxRange,
			const std::vector<Glyph>& glyphs,
			const std::vector<msdfgen::Shape>& shapes,
			rdr::BitmapView<rdr::RGBAPixel<uint8>> out_bitmap,
			mt::ThreadPool& threadPool);

	private:
		FT_FaceRec_* m_Face;
		uint32 m_MaxWidth;
		uint32 m_Res;
		double m_Padding;
		double m_Scale = 1.0;
	};

	class FontAtlas : public IAsset
	{
	public:
		using IAsset::IAsset;

		APOLLO_API FontAtlas(
			FT_FaceRec_* faceHandle,
			uint32 pixelSize,
			GlyphRange range,
			std::vector<Glyph> glyphs,
			std::vector<uint32> indices,
			Texture2D texture);

		FontAtlas(const FontAtlas&) = delete;
		FontAtlas(FontAtlas&& other) noexcept
			: m_FaceHandle(std::exchange(other.m_FaceHandle, nullptr))
			, m_Range(std::exchange(other.m_Range, { 0, 0 }))
			, m_Texture(std::move(other.m_Texture))
			, m_Glyphs(std::move(other.m_Glyphs))
			, m_Indices(std::move(other.m_Indices))
			, m_PixelSize(other.m_PixelSize)
		{}

		FontAtlas& operator=(const FontAtlas&) = delete;
		FontAtlas& operator=(FontAtlas&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		void Swap(FontAtlas& other)
		{
			std::swap(m_FaceHandle, other.m_FaceHandle);
			std::swap(m_Range, other.m_Range);
			std::swap(m_Texture, other.m_Texture);
			std::swap(m_Glyphs, other.m_Glyphs);
			std::swap(m_Indices, other.m_Indices);
			std::swap(m_PixelSize, other.m_PixelSize);
		}

		APOLLO_API ~FontAtlas();

		[[nodiscard]] const rdr::Texture2D& GetTexture() const noexcept { return m_Texture; }
		[[nodiscard]] const Glyph* GetGlyph(char32_t ch, char32_t fallback = U' ') const noexcept
		{
			if (const auto* g = FindGlyph(ch))
				return g;
			return FindGlyph(fallback);
		}
		[[nodiscard]] uint32 GetPixelSize() const noexcept { return m_PixelSize; }

		[[nodiscard]] APOLLO_API float2
		GetKerning(const Glyph& left, const Glyph& right) const noexcept;

		[[nodiscard]] APOLLO_API float2 MeasureText(
			std::string_view txt,
			const TextStyle& style,
			char32_t fallback = U' ') const noexcept;

		GET_ASSET_TYPE_IMPL(EAssetType::FontAtlas);

	private:
		APOLLO_API const Glyph* FindGlyph(char32_t ch) const noexcept;

		FT_FaceRec_* m_FaceHandle = nullptr;
		GlyphRange m_Range;
		rdr::Texture2D m_Texture;
		std::vector<Glyph> m_Glyphs;
		std::vector<uint32> m_Indices;
		uint32 m_PixelSize = 64;

		friend struct editor::AssetHelper<FontAtlas>;
	};
} // namespace apollo::rdr::txt