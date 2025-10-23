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

	template <class ShaderType>
	apollo::EAssetLoadResult LoadShader(
		ShaderType& out_shader,
		const apollo::AssetMetadata& metadata)
	{
		using namespace apollo;
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
		const bool isByteCode = IsSpirvByteCode(data.get(), len);
#endif
		if (isByteCode)
		{
			out_shader = ShaderType{ metadata.m_Id, data.get(), len };
		}
		else
		{
			std::string_view src{ data.get(), len };
			out_shader = ShaderType::CompileFromSource(metadata.m_Id, src);
		}
		return out_shader ? EAssetLoadResult::Success : EAssetLoadResult::Failure;
	}
} // namespace

namespace apollo::editor {
	EAssetLoadResult LoadVertexShader(IAsset& out_asset, const AssetMetadata& metadata)
	{
		return LoadShader(static_cast<rdr::VertexShader&>(out_asset), metadata);
	}
	EAssetLoadResult LoadFragmentShader(IAsset& out_asset, const AssetMetadata& metadata)
	{
		return LoadShader(static_cast<rdr::FragmentShader&>(out_asset), metadata);
	}
} // namespace apollo::editor
