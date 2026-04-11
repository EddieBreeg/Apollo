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
	struct ShaderStageT;
	template <>
	struct ShaderStageT<apollo::rdr::VertexShader>
	{
		static constexpr const char Name[] = "vertex";
	};
	template <>
	struct ShaderStageT<apollo::rdr::FragmentShader>
	{
		static constexpr const char Name[] = "fragment";
	};

	template <size_t N>
	Slang::ComPtr<slang::IEntryPoint> DeduceEntryPoint(
		slang::IModule& module,
		const char (&stageName)[N],
		apollo::rdr::ShaderCompiler& compiler)
	{
		Slang::ComPtr<slang::IEntryPoint> main;

		for (int32 i = 0; i < module.getDefinedEntryPointCount(); ++i)
		{
			Slang::ComPtr<slang::IEntryPoint> ep;
			module.getDefinedEntryPoint(i, ep.writeRef());
			if (module.getName() == std::string_view{ "main" })
			{
				main = ep;
			}
			auto* attr = compiler.FindAttributeByName(*ep->getFunctionReflection(), "shader");
			if (!attr)
				continue;

			size_t len = 0;
			if (std::string_view{ attr->getArgumentValueString(0, &len), len } == stageName)
				return ep;
		}
		return main;
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
		auto pathString = metadata.m_FilePath.string();

		if (!inFile.is_open())
		{
			APOLLO_LOG_ERROR(
				"Failed to load shader {}({}) from {}: {}",
				metadata.m_Name,
				metadata.m_Id,
				pathString,
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
				pathString);
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
				pathString,
				GetErrnoMessage(errno));
			return false;
		}

		const bool isByteCode = IsSlangByteCode(data->GetPtrAs<const char>(), len);
		slang::IModule* module = nullptr;
		Slang::ComPtr<slang::IBlob> diagnostics;

		if (isByteCode)
		{
			module = compiler.LoadFromIntermediate(
				metadata.m_Name.c_str(),
				data,
				pathString.c_str(),
				diagnostics.writeRef());
		}
		else
		{
			module = compiler.LoadModuleFromSource(
				data,
				metadata.m_Name.c_str(),
				pathString.c_str(),
				diagnostics.writeRef());
		}

		if (!module)
		{
			APOLLO_LOG_ERROR(
				"Failed to load shader {}\n{:.{}}",
				metadata.m_Name,
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize());
			return false;
		}
		auto ep = DeduceEntryPoint(*module, ShaderStageT<ShaderType>::Name, compiler);
		if (!ep)
		{
			APOLLO_LOG_ERROR("Failed to deduce entry point for shader {}", metadata.m_Name);
			return false;
		}
		out_shader = ShaderType{ metadata.m_Id, *module, *ep };
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
