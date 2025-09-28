#include "AssetImporters.hpp"
#include "core/Assert.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/Asset.hpp>
#include <asset/AssetManager.hpp>
#include <core/Enum.hpp>
#include <core/Errno.hpp>
#include <core/Log.hpp>
#include <core/Map.hpp>
#include <core/StringHash.hpp>
#include <core/ULIDFormatter.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <rendering/Renderer.hpp>
#include <rendering/Texture.hpp>
#include <string_view>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace {
	const brk::StringHashMap<brk::EAssetType> g_AssetTypeMap{
		{ brk::StringHash{ "texture2d" }, brk::EAssetType::Texture2D },
		{ brk::StringHash{ "shader" }, brk::EAssetType::Shader },
		{ brk::StringHash{ "material" }, brk::EAssetType::Material },
	};

	struct Parser
	{
		std::string_view m_Line;
		std::string_view GetNext()
		{
			if (m_Line.empty())
				return {};

			size_t endPos = m_Line.npos;
			size_t suffixLen = 0;
			size_t prefixLen = 0;

			if (m_Line[0] == '"')
			{
				prefixLen = 1;
				endPos = m_Line.find('"', 1);
				if (endPos == m_Line.npos)
				{
					BRK_LOG_ERROR("CSV sequence is malformed: {}", m_Line);
					return (m_Line = {});
				}
				if (endPos == (m_Line.length() - 1))
				{
					suffixLen = 1;
				}
				else if (m_Line[endPos + 1] == ',')
				{
					BRK_LOG_ERROR("CSV sequence is malformed: {}", m_Line);
					return (m_Line = {});
				}
				else
				{
					suffixLen = 2;
				}
			}
			else
			{
				endPos = m_Line.find(',');
				suffixLen = endPos < m_Line.size();
			}

			std::string_view val = m_Line.substr(prefixLen, endPos - prefixLen);
			if (endPos < m_Line.length())
				m_Line.remove_prefix(std::min(endPos + suffixLen, m_Line.length()));
			else
				m_Line = {};
			return val;
		}

#define CHECK_VAL(val, msg)                                                                        \
	if ((val).empty())                                                                             \
	{                                                                                              \
		BRK_LOG_ERROR("Failed to parse asset metadata: " msg);                                     \
		return false;                                                                              \
	}

		bool ParseMetadata(brk::AssetMetadata& out_metadata, const std::filesystem::path& assetRoot)
		{
			std::string_view val = GetNext();
			CHECK_VAL(val, "missing asset ULID");
			out_metadata.m_Id = brk::ULID::FromString(val);
			if (!out_metadata.m_Id)
			{
				BRK_LOG_ERROR("Failed to parse asset metadata: invalid ULID {}", val);
				return false;
			}
			val = GetNext();
			CHECK_VAL(val, "missing asset type");
			{
				const auto it = g_AssetTypeMap.find(brk::StringHash{ val });
				if (it == g_AssetTypeMap.end())
				{
					BRK_LOG_ERROR("Failed to parse asset metadata: invalid type {}", val);
					return false;
				}
				out_metadata.m_Type = it->second;
			}

			val = GetNext();
			CHECK_VAL(val, "missing asset name");
			out_metadata.m_Name = val;

			val = GetNext();
			CHECK_VAL(val, "missing asset path");
			out_metadata.m_FilePath = assetRoot / val;

			return true;
		}
	};
#undef CHECK_VAL
} // namespace

namespace brk::editor {
	bool ImporteAssetMetadata(
		const std::filesystem::path& assetRootPath,
		ULIDMap<AssetMetadata>& out_metadata)
	{
		(void)out_metadata;
		const std::filesystem::path filePath = assetRootPath / "metadata.csv";

		std::ifstream inFile{
			assetRootPath / "metadata.csv",
			std::ios::binary,
		};
		std::string line;
		if (!inFile.is_open())
		{
			BRK_LOG_ERROR("Failed to load {}: {}", filePath.string(), GetErrnoMessage(errno));
			return false;
		}
		std::getline(inFile, line);
		for (; inFile; std::getline(inFile, line))
		{
			if (line.empty() || line[0] == '\r')
				continue;

			Parser parser{ line };
			if (line.back() == '\r')
				parser.m_Line.remove_suffix(1);

			AssetMetadata metadata;
			if (!parser.ParseMetadata(metadata, assetRootPath))
				continue;

			auto res = out_metadata.try_emplace(metadata.m_Id, std::move(metadata));
			if (res.second)
				continue;
			BRK_LOG_ERROR(
				"ULID {} is present multiple times in the metadata file. Only the first asset will "
				"be registered",
				metadata.m_Id);
		}
		return true;
	}

	EAssetLoadResult ImportTexture2d(IAsset& out_texture, const AssetMetadata& metadata)
	{
		const auto pathStr = metadata.m_FilePath.string();
		int32 width = 0, height = 0;
		int32 numChannels = 0;
		uint8* data = stbi_load(pathStr.c_str(), &width, &height, &numChannels, 0);
		if (!data)
		{
			BRK_LOG_ERROR("Failed to load texture from {}: {}", pathStr, stbi_failure_reason());
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
		rdr::GPUDevice& device = rdr::Renderer::GetInstance()->GetDevice();
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
		BRK_ASSERT(transferBuffer, "Failed to create transfer buffer: {}", SDL_GetError());
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
			for (uint32 i = 0; i < uint32(width * height); ++i)
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
			.pixels_per_row = uint32(width),
			.rows_per_layer = uint32(height),
		};
		const SDL_GPUTextureRegion region{
			.texture = tex.m_Handle,
			.x = 0,
			.y = 0,
			.w = uint32(width),
			.h = uint32(height),
			.d = 1,
		};

		SDL_UploadToGPUTexture(AssetLoader::GetCurrentCopyPass(), &transferInfo, &region, false);

		SDL_ReleaseGPUTransferBuffer(device.GetHandle(), transferBuffer);

		return EAssetLoadResult::Success;
	}
} // namespace brk::editor