#include "AssetLoaders.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/Assert.hpp>
#include <core/NumConv.hpp>
#include <rendering/Context.hpp>
#include <rendering/Texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace apollo::editor {
	template <>
	EAssetLoadResult AssetHelper<rdr::Texture2D>::Load(
		IAsset& out_texture,
		const AssetMetadata& metadata)
	{
		const auto pathStr = metadata.m_FilePath.string();
		int32 width = 0, height = 0;
		int32 numChannels = 0;
		uint8* data = stbi_load(pathStr.c_str(), &width, &height, &numChannels, 0);
		if (!data)
		{
			APOLLO_LOG_ERROR("Failed to load texture from {}: {}", pathStr, stbi_failure_reason());
			return EAssetLoadResult::Failure;
		}

		rdr::EPixelFormat format = rdr::EPixelFormat::Invalid;
		uint32 pixelSize = 0;
		switch (numChannels)
		{
		case 1:
			format = rdr::EPixelFormat::R8_UNorm;
			pixelSize = 1;
			break;
		case 2:
			format = rdr::EPixelFormat::RG8_UNorm;
			pixelSize = 2;
			break;
		case 3: [[fallthrough]];
		case 4:
			format = rdr::EPixelFormat::RGBA8_UNorm;
			pixelSize = 4;
			break;
		default: break;
		}

		rdr::Texture2D& tex = static_cast<rdr::Texture2D&>(out_texture);
		rdr::GPUDevice& device = rdr::Context::GetInstance()->GetDevice();
		tex = rdr::Texture2D(
			metadata.m_Id,
			rdr::TextureSettings{
				.m_Width = static_cast<uint32>(width),
				.m_Height = static_cast<uint32>(height),
				.m_Format = format,
				.m_Usage = rdr::ETextureUsageFlags::Sampled,
			});

		DEBUG_CHECK(tex.m_Handle)
		{
			return EAssetLoadResult::Failure;
		}

		SDL_GPUTransferBufferCreateInfo transferBufferInfo{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = width * height * pixelSize,
		};
		SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
			device.GetHandle(),
			&transferBufferInfo);
		APOLLO_ASSERT(transferBuffer, "Failed to create transfer buffer: {}", SDL_GetError());
		void* bufMem = SDL_MapGPUTransferBuffer(device.GetHandle(), transferBuffer, false);

		if (numChannels != 3)
		{
			std::memcpy(bufMem, data, transferBufferInfo.size);
		}
		else
		{
			// RGB isn't supported for GPU textures, we need to create the 4th channel ourself
			struct RGBAPixel
			{
				uint8 r, g, b, a;
			};
			for (uint32 i = 0; i < NumCast<uint32>(width * height); ++i)
			{
				static_cast<RGBAPixel*>(bufMem)[i] = RGBAPixel{
					data[3 * i],
					data[3 * i + 1],
					data[3 * i + 2],
					255,
				};
			}
		}

		SDL_UnmapGPUTransferBuffer(device.GetHandle(), transferBuffer);
		stbi_image_free(data);

		SDL_GPUTextureTransferInfo transferInfo{
			.transfer_buffer = transferBuffer,
			.offset = 0,
			.pixels_per_row = NumCast<uint32>(width),
			.rows_per_layer = NumCast<uint32>(height),
		};
		const SDL_GPUTextureRegion region{
			.texture = tex.m_Handle,
			.x = 0,
			.y = 0,
			.w = NumCast<uint32>(width),
			.h = NumCast<uint32>(height),
			.d = 1,
		};

		SDL_UploadToGPUTexture(AssetLoader::GetCurrentCopyPass(), &transferInfo, &region, false);

		SDL_ReleaseGPUTransferBuffer(device.GetHandle(), transferBuffer);

		return EAssetLoadResult::Success;
	}
} // namespace apollo::editor