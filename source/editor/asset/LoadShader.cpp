#include "AssetHelper.hpp"
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Errno.hpp>
#include <core/Log.hpp>
#include <fstream>
#include <rendering/Shader.hpp>
#include <tools/ShaderCompiler.hpp>

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

	Slang::ComPtr<slang::IBlob> BuildShader(
		slang::IBlob* source,
		const char* name,
		SlangStage stage,
		apollo::rdr::ShaderCompiler& compiler)
	{
		Slang::ComPtr<slang::IBlob> res, diagnostics;
		auto* module = compiler.LoadModuleFromSource(source, name, nullptr, diagnostics.writeRef());
		if (!module)
		{
			std::string_view msg{
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize(),
			};
			APOLLO_LOG_ERROR("Failed to compile shader '{}':\n{}", name, msg);
			return res;
		}
		res = compiler.GetTargetCode(*module, "main", stage, diagnostics.writeRef());
		if (!res)
		{
			std::string_view msg{
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize(),
			};
			APOLLO_LOG_ERROR("Failed to link shader '{}':\n{}", name, msg);
		}
		return res;
	}

	template <class S>
	struct ShaderStageT;
	template <>
	struct ShaderStageT<apollo::rdr::VertexShader>
	{
		static constexpr SlangStage Value = SLANG_STAGE_VERTEX;
	};
	template <>
	struct ShaderStageT<apollo::rdr::FragmentShader>
	{
		static constexpr SlangStage Value = SLANG_STAGE_FRAGMENT;
	};

	template <class ShaderType>
	bool LoadShader(
		ShaderType& out_shader,
		const apollo::AssetMetadata& metadata,
		apollo::rdr::ShaderCompiler& compiler)
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
			return false;
		}
		size_t len = inFile.tellg();
		if (!len)
		{
			APOLLO_LOG_ERROR(
				"Failed to load shader {}({}) from {}: empty file",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string());
			return false;
		}
		using Blob = rdr::ShaderCompiler::Blob;
		Slang::ComPtr data{ Blob::Allocate(inFile.tellg()) };
		inFile.seekg(0, std::ios::beg);

		if (inFile.read(data->GetPtrAs<char>(), len).fail())
		{
			APOLLO_LOG_ERROR(
				"Failed to load shader {}({}) from {}: {}",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return false;
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
			const Slang::ComPtr<slang::IBlob> code = BuildShader(
				data,
				metadata.m_Name.c_str(),
				ShaderStageT<ShaderType>::Value,
				compiler);
			if (code)
			{
				out_shader = ShaderType{
					metadata.m_Id,
					code->getBufferPointer(),
					code->getBufferSize(),
				};
			}
		}
		return out_shader;
	}
} // namespace

namespace apollo::editor {
	template <>
	AssetLoadTask AssetHelper<rdr::VertexShader>::LoadAsync(
		IAsset& out_asset,
		const AssetMetadata& metadata)
	{
		co_return LoadShader(
			static_cast<rdr::VertexShader&>(out_asset),
			metadata,
			apollo::App::GetShaderCompiler());
	}

	template <>
	AssetLoadTask AssetHelper<rdr::FragmentShader>::LoadAsync(
		IAsset& out_asset,
		const AssetMetadata& metadata)
	{
		co_return LoadShader(
			static_cast<rdr::FragmentShader&>(out_asset),
			metadata,
			apollo::App::GetShaderCompiler());
	}
} // namespace apollo::editor
