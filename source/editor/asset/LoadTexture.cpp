#include "AssetHelper.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/Assert.hpp>
#include <core/NumConv.hpp>
#include <rendering/Context.hpp>
#include <rendering/Texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace apollo::editor {
	AssetLoadTask AssetHelper<apollo::rdr::Texture2D>::LoadAsync(
		IAsset& out_asset,
		const AssetMetadata& metadata)
	{
		co_return DoLoad(static_cast<rdr::Texture2D&>(out_asset), metadata);
	}

	bool AssetHelper<apollo::rdr::Texture2D>::DoLoad(
		rdr::Texture2D& out_texture,
		const AssetMetadata& metadata)
	{
		int32 width = 0, height = 0;
		int32 numChannels = 0;
		uint8* data = stbi_load(metadata.m_FilePath.c_str(), &width, &height, &numChannels, 0);
		if (!data)
		{
			APOLLO_LOG_ERROR("Failed to load texture from {}: {}", metadata.m_FilePath, stbi_failure_reason());
			return false;
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

		rdr::GPUDevice& device = rdr::Context::GetInstance()->GetDevice();
		out_texture = rdr::Texture2D(
			metadata.m_Id,
			rdr::TextureSettings{
				.m_Width = static_cast<uint32>(width),
				.m_Height = static_cast<uint32>(height),
				.m_Format = format,
				.m_Usage = rdr::ETextureUsageFlags::Sampled,
			});

		DEBUG_CHECK(out_texture.m_Handle)
		{
			return false;
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
			.texture = out_texture.m_Handle,
			.x = 0,
			.y = 0,
			.w = NumCast<uint32>(width),
			.h = NumCast<uint32>(height),
			.d = 1,
		};

		SDL_UploadToGPUTexture(AssetLoader::GetCurrentCopyPass(), &transferInfo, &region, false);

		SDL_ReleaseGPUTransferBuffer(device.GetHandle(), transferBuffer);

		return true;
	}

	void AssetHelper<rdr::Texture2D>::Swap(IAsset& lhs, IAsset& rhs)
	{
		static_cast<rdr::Texture2D&>(lhs).Swap(static_cast<rdr::Texture2D&>(rhs));
	}
} // namespace apollo::editor