#include "AssetHelper.hpp"
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Errno.hpp>
#include <core/Log.hpp>
#include <fstream>
#include <rendering/Shader.hpp>
#include <tools/ShaderCompiler.hpp>

namespace {
	bool IsSlangByteCode(const char* data, size_t len)
	{
		if (len < 4)
			return false;
		return std::string_view{ data, 4 } == "RIFF";
	}

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

		const bool isByteCode = IsSlangByteCode(data->GetPtrAs<const char>(), len);
		if (isByteCode)
		{
			Slang::ComPtr<slang::IBlob> diagnostics;
			auto* module = compiler.LoadFromIntermediate(
				metadata.m_Name.c_str(),
				data,
				nullptr,
				diagnostics.writeRef());
			if (!module)
			{
				APOLLO_LOG_ERROR(
					"Failed to load shader {} from byte code\n{:.{}}",
					metadata.m_Name,
					static_cast<const char*>(diagnostics->getBufferPointer()),
					diagnostics->getBufferSize());
				return false;
			}
			out_shader = ShaderType{ metadata.m_Id, *module };
		}
		else
		{
			out_shader =  ShaderType::CompileFromSource(metadata.m_Id, data);
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
