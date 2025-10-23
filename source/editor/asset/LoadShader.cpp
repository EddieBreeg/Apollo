#include "AssetLoaders.hpp"
#include <asset/AssetManager.hpp>
#include <core/Errno.hpp>
#include <core/Log.hpp>
#include <core/ULIDFormatter.hpp>
#include <fstream>
#include <rendering/Shader.hpp>

namespace {
#ifdef APOLLO_VULKAN
	bool IsSpirvByteCode(const void* data, size_t len)
	{
		if (len % 4)
			return false;
		uint32 w = *static_cast<const uint32_t*>(data);
		// compare to SPIRV magic number in both little and big endian
		return (w == 0x07230203) || (w == 0x03022307);
	}
#endif
} // namespace

namespace apollo::editor {
	EAssetLoadResult LoadShader(IAsset& out_asset, const AssetMetadata& metadata)
	{
		std::ifstream inFile{
			metadata.m_FilePath,
			std::ios::binary | std::ios::ate,
		};
		if (!inFile.is_open())
		{
			APOLLO_LOG_ERROR(
				"Failed to load shader {}({}) from {}: {}",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return EAssetLoadResult::Failure;
		}
		size_t len = inFile.tellg();
		if (!len)
		{
			APOLLO_LOG_ERROR(
				"Failed to load shader {}({}) from {}: empty file",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string());
			return EAssetLoadResult::Failure;
		}
		inFile.seekg(0, std::ios::beg);
		std::unique_ptr data = std::make_unique<char[]>(len);
		if (inFile.read(data.get(), len).fail())
		{
			APOLLO_LOG_ERROR(
				"Failed to load shader {}({}) from {}: {}",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return EAssetLoadResult::Failure;
		}
#ifdef APOLLO_VULKAN
		if (!IsSpirvByteCode(data.get(), len))
		{
			APOLLO_LOG_ERROR(
				"Failed to load shader {}({}) from {}: not a SPIRV binary",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string());
		}
#endif

		switch (metadata.m_Type)
		{
		case apollo::EAssetType::VertexShader:
		{
			rdr::VertexShader& shader = dynamic_cast<rdr::VertexShader&>(out_asset);
			shader = rdr::VertexShader(metadata.m_Id, data.get(), len);
			return shader ? EAssetLoadResult::Success : EAssetLoadResult::Failure;
		}

		case apollo::EAssetType::FragmentShader:
		{
			rdr::FragmentShader& shader = dynamic_cast<rdr::FragmentShader&>(out_asset);
			shader = rdr::FragmentShader(metadata.m_Id, data.get(), len);
			return shader ? EAssetLoadResult::Success : EAssetLoadResult::Failure;
		}

		default: return EAssetLoadResult::Failure;
		}
	}
} // namespace apollo::editor
