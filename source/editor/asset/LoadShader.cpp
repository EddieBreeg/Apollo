#include "AssetLoaders.hpp"
#include <asset/AssetManager.hpp>
#include <core/Errno.hpp>
#include <core/Log.hpp>
#include <core/ULIDFormatter.hpp>
#include <fstream>
#include <rendering/Shader.hpp>

#ifdef BRK_VULKAN
#include <spirv_reflect.h>
namespace {
	[[maybe_unused]] const char* GetSpirvReflectErrorMsg(SpvReflectResult code)
	{
		constexpr const char* msg[] = {
			"Success",
			"Not Ready",
			"Error Parse Failed",
			"Error Alloc Failed",
			"Error Range Exceeded",
			"Error Null Pointer",
			"Error Internal Error",
			"Error Count Mismatch",
			"Error Element Not Found",
			"Error Spirv Invalid Code Size",
			"Error Spirv Invalid Magic Number",
			"Error Spirv Unexpected Eof",
			"Error Spirv Invalid Id Reference",
			"Error Spirv Set Number Overflow",
			"Error Spirv Invalid Storage Class",
			"Error Spirv Recursion",
			"Error Spirv Invalid Instruction",
			"Error Spirv Unexpected Block Data",
			"Error Spirv Invalid Block Member Reference",
			"Error Spirv Invalid Entry Point",
			"Error Spirv Invalid Execution Mode",
			"Error Spirv Max Recursive Exceeded",
		};
		DEBUG_CHECK(code >= 0 && code < STATIC_ARRAY_SIZE(msg))
		{
			return "Unknown";
		}
		return msg[code];
	}
	void GetSpirvShaderInfo(SpvReflectShaderModule module, brk::rdr::ShaderInfo& out_info)
	{
		using namespace brk::rdr;
		out_info.m_EntryPoint = module.entry_point_name;
		switch (module.shader_stage)
		{
		case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: out_info.m_Stage = EShaderStage::Vertex; break;
		case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
			out_info.m_Stage = EShaderStage::Fragment;
			break;
		default: [[unlikely]] out_info.m_Stage = EShaderStage::Invalid; return;
		}

		for (uint32 i = 0; i < module.descriptor_binding_count; ++i)
		{
			switch (module.descriptor_bindings[i].descriptor_type)
			{
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: ++out_info.m_NumStorageBuffers; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: ++out_info.m_NumUniformBuffers; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: ++out_info.m_NumSamplers; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: ++out_info.m_NumStorageTextures; break;
			default: break;
			}
		}
	}
} // namespace
#endif

namespace brk::editor {
	EAssetLoadResult LoadShader(IAsset& out_asset, const AssetMetadata& metadata)
	{
		std::ifstream inFile{
			metadata.m_FilePath,
			std::ios::binary | std::ios::ate,
		};
		if (!inFile.is_open())
		{
			BRK_LOG_ERROR(
				"Failed to load shader {}({}) from {}: {}",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return EAssetLoadResult::Failure;
		}
		size_t len = inFile.tellg();
		inFile.seekg(0, std::ios::beg);
		std::unique_ptr data = std::make_unique<char[]>(len);
		if (inFile.read(data.get(), len).fail())
		{
			BRK_LOG_ERROR(
				"Failed to load shader {}({}) from {}: {}",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return EAssetLoadResult::Failure;
		}

		rdr::Shader& shader = dynamic_cast<rdr::Shader&>(out_asset);
		rdr::ShaderInfo info;
#ifdef BRK_VULKAN
		SpvReflectShaderModule reflectModule{};
		auto res = spvReflectCreateShaderModule(len, data.get(), &reflectModule);
		if (res != SPV_REFLECT_RESULT_SUCCESS) [[unlikely]]
		{
			BRK_LOG_ERROR(
				"Failed to inspect SPIRV shader {}({}): {}",
				metadata.m_Name,
				metadata.m_Id,
				GetSpirvReflectErrorMsg(res));
			return EAssetLoadResult::Failure;
		}
		GetSpirvShaderInfo(reflectModule, info);
#endif
		if (info.m_Stage == rdr::EShaderStage::Invalid) [[unlikely]]
		{
			BRK_LOG_ERROR("Shader {}({}) has unsupported stage", metadata.m_Name, metadata.m_Id);
			goto LOAD_END;
		}

		shader = rdr::Shader(metadata.m_Id, info, data.get(), len);

	LOAD_END:
#ifdef BRK_VULKAN
		spvReflectDestroyShaderModule(&reflectModule);
#endif

		return shader ? EAssetLoadResult::Success : EAssetLoadResult::Failure;
	}

} // namespace brk::editor
