#include "AssetLoaders.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/AssetLoader.hpp>
#include <asset/AssetManager.hpp>
#include <core/Assert.hpp>
#include <core/Errno.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <filesystem>
#include <freetype/freetype.h>
#include <fstream>
#include <msdfgen.h>
#include <rendering/Renderer.hpp>
#include <rendering/text/FontAtlas.hpp>

namespace {
	struct FreetypeContext
	{
		operator FT_Library() noexcept
		{
			if (m_Handle) [[likely]]
				return m_Handle;
			const FT_Error err = FT_Init_FreeType(&m_Handle);
			BRK_ASSERT(err == FT_Err_Ok, "Failed to init Freetype: {}", FT_Error_String(err));
			return m_Handle;
		}

	private:
		FT_Library m_Handle = nullptr;
	};

	thread_local FreetypeContext g_Freetype;
} // namespace

namespace brk::json {
	bool Visit(brk::rdr::txt::GlyphRange& out_range, const nlohmann::json& json, const char* key)
	{
		const auto node = json.find(key);
		if (node == json.end())
			return true;

		if (!Visit(out_range.m_First, *node, "first", true))
			return false;
		return Visit(out_range.m_Last, *node, "last", true);
	}

} // namespace brk::json

namespace brk::editor {
	EAssetLoadResult LoadFont(IAsset& out_asset, const AssetMetadata& metadata)
	{
		using namespace rdr::txt;
		auto& atlas = dynamic_cast<FontAtlas&>(out_asset);
		std::ifstream jsonFile{ metadata.m_FilePath, std::ios::binary };
		if (!jsonFile.is_open())
		{
			BRK_LOG_ERROR(
				"Failed to open {}: {}",
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return EAssetLoadResult::Failure;
		}
		const auto json = nlohmann::json::parse(jsonFile, nullptr, false);
		if (json.is_discarded())
		{
			BRK_LOG_ERROR("Failed to parse {} as JSON", metadata.m_FilePath.string());
			return EAssetLoadResult::Failure;
		}

		if (!json::Visit(atlas.m_Range, json, "range"))
		{
			BRK_LOG_ERROR("Failed to parse glyph range from JSON");
			return EAssetLoadResult::Failure;
		}
		if (atlas.m_Range.m_Last < atlas.m_Range.m_First)
		{
			BRK_LOG_WARN(
				"Glyph range has invalid bounds {}:{}",
				uint32(atlas.m_Range.m_First),
				uint32(atlas.m_Range.m_Last));
			std::swap(atlas.m_Range.m_First, atlas.m_Range.m_Last);
		}

		if (!json::Visit(atlas.m_PixelSize, json, "pixelSize", true))
			BRK_LOG_WARN("Failed to parse pixel size from JSON");

		std::string_view fontPath;
		if (!json::Visit(fontPath, json, "fontFile"))
		{
			BRK_LOG_ERROR("Failed to get font file path from JSON");
			return EAssetLoadResult::Failure;
		}
		FT_Error err = FT_New_Face(g_Freetype, fontPath.data(), 0, &atlas.m_FaceHandle);
		if (err)
		{
			BRK_LOG_ERROR("Failed to load font file {}: {}", fontPath, FT_Error_String(err));
			return EAssetLoadResult::Failure;
		}
		double pxRange = 10.0;
		double emPadding = 1.0 / atlas.m_PixelSize;
		json::Visit(pxRange, json, "pixelRange", true);
		json::Visit(emPadding, json, "padding", true);

		AtlasGenerator generator{
			atlas.m_FaceHandle,
			atlas.m_PixelSize * 16,
			atlas.m_PixelSize,
			emPadding,
		};
		std::vector<msdfgen::Shape> shapes;
		const glm::uvec2 texSize = generator.LoadGlyphRange(
			atlas.m_Range,
			atlas.m_Glyphs,
			shapes,
			atlas.m_Indices);
		atlas.m_Texture = rdr::Texture2D(
			rdr::TextureSettings{
				.m_Width = texSize.x,
				.m_Height = texSize.y,
			});
		const uint32 pixelCount = texSize.x * texSize.y;

		SDL_GPUCopyPass* const copyPass = AssetLoader::GetCurrentCopyPass();
		const SDL_GPUTransferBufferCreateInfo tBufferInfo{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = uint32(sizeof(rdr::RGBAPixel<uint8>) * pixelCount),
		};
		SDL_GPUDevice* device = rdr::Renderer::GetInstance()->GetDevice().GetHandle();

		SDL_GPUTransferBuffer* transferBuf = SDL_CreateGPUTransferBuffer(device, &tBufferInfo);
		auto* buf = (rdr::RGBAPixel<uint8>*)SDL_MapGPUTransferBuffer(device, transferBuf, true);
		BRK_ASSERT(buf, "Failed to map transfer buffer: {}", SDL_GetError());

		generator.Rasterize(pxRange, atlas.m_Glyphs, shapes, { buf, texSize.x, texSize.y });
		const SDL_GPUTextureTransferInfo srcLoc{
			.transfer_buffer = transferBuf,
			.pixels_per_row = texSize.x,
			.rows_per_layer = texSize.y,
		};
		const SDL_GPUTextureRegion destRegion{
			.texture = atlas.m_Texture.GetHandle(),
			.w = texSize.x,
			.h = texSize.y,
			.d = 1,
		};
		SDL_UploadToGPUTexture(copyPass, &srcLoc, &destRegion, true);

		SDL_ReleaseGPUTransferBuffer(device, transferBuf);

		return EAssetLoadResult::Success;
	}
} // namespace brk::editor