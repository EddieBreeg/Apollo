#include "ShaderCompiler.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

struct Options
{
	const char* m_FileName = nullptr;
	bool m_ShowHelp = false;
	SlangCompileTarget m_Target = SlangCompileTarget::SLANG_SPIRV;
	SlangStage m_Stage = SLANG_STAGE_NONE;
	std::vector<const char*> m_IncludePath = { "." };
	const char* m_EntryPointName = "main";
	const char* m_OutPath = nullptr;
};

namespace argp {
	bool ParseValue(SlangCompileTarget& out_val, std::string_view str) noexcept
	{
		if (str == "vulkan")
		{
			out_val = SLANG_SPIRV;
		}
		else if (str == "d3d12")
		{
			out_val = SLANG_DXIL;
		}
		else
		{
			return false;
		}
		return true;
	}
	bool ParseValue(SlangStage& out_val, std::string_view str) noexcept
	{
		if (str == "vertex")
		{
			out_val = SLANG_STAGE_VERTEX;
		}
		else if (str == "pixel" || str == "fragment")
		{
			out_val = SLANG_STAGE_FRAGMENT;
		}
		else if (str == "compute")
		{
			out_val = SLANG_STAGE_COMPUTE;
		}
		else
		{
			return false;
		}
		return true;
	}

	bool ParseValue(std::vector<const char*>& out_vec, const char* val)
	{
		out_vec.emplace_back(val);
		return true;
	}
} // namespace argp

#include "ArgParse.hpp"
namespace {
	struct PathInfo
	{
		const char* m_FileName = nullptr;
		const char* m_Ext = nullptr;
		const char* m_End = nullptr;
	};

	PathInfo GetPathInfo(const char* path)
	{
		PathInfo info{
			.m_FileName = path,
			.m_End = path,
		};

		while (const char c = *info.m_End)
		{
			switch (c)
			{
			case '.': info.m_Ext = info.m_End; break;
			case '/':
			case '\\':
				info.m_FileName = info.m_End + 1;
				info.m_Ext = nullptr;
				break;
			default: break;
			}
			++info.m_End;
		}
		return info;
	}

	using Blob = apollo::rdr::ShaderCompiler::Blob;

	Slang::ComPtr<Blob> LoadFile(const char* path)
	{
		std::ifstream file{ path, std::ios::ate | std::ios::binary };
		if (!file.is_open())
		{
			std::cerr << "Failed to open " << path << '\n';
			return {};
		}
		Blob* data = Blob::Allocate(file.tellg());
		file.seekg(0, std::ios::beg);
		if (!file.read(data->GetPtrAs<char>(), data->GetSize()))
		{
			std::cerr << "Failed to read " << path << '\n';
			return {};
		}
		return Slang::ComPtr{ data };
	}
} // namespace

int main(int argc, const char* const* argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: shaderc [options...] <file>\n";
		return 1;
	}
	Options options;

	options.m_FileName = argv[argc - 1];
	std::span args{ argv + 1, size_t(argc - 2) };

	slang::CompilerOptionEntry compilerOptions[] = {
		slang::CompilerOptionEntry{
			.name = slang::CompilerOptionName::Language,
			.value =
				slang::CompilerOptionValue{
					.intValue0 = SLANG_SOURCE_LANGUAGE_SLANG,
				},
		},
	};

	using argp::NamedArgument;

	try
	{
		using NamedArgs = argp::ArgList<
			NamedArgument{ &Options::m_Target, "-T" },
			NamedArgument{ &Options::m_IncludePath, "-I" },
			NamedArgument{ &Options::m_EntryPointName, "-E" },
			NamedArgument{ &Options::m_Stage, "-S" },
			NamedArgument{ &Options::m_OutPath, "-o" },
			NamedArgument{ &Options::m_ShowHelp, "--help" },
			NamedArgument{ &Options::m_ShowHelp, "-h" }>;
		NamedArgs::Parse(options, args);
	}
	catch (const argp::MissingArgumentError& err)
	{
		std::cerr << "Missing value for argument " << err.m_Name << '\n';
		return 1;
	}
	catch (const argp::UnknownArgumentError& err)
	{
		std::cerr << "Unknown argument: '" << err.m_Arg << "'\n";
		return 1;
	}
	catch (const argp::InvalidValueError& err)
	{
		std::cerr << "Value '" << err.m_Value << "' is invalid for '" << err.m_Name << "'\n";
		return 1;
	}

	const PathInfo pathInfo = GetPathInfo(options.m_FileName);
	if (pathInfo.m_Ext && !strcmp(pathInfo.m_Ext, ".hlsl"))
	{
		compilerOptions[0].value.intValue0 = SLANG_SOURCE_LANGUAGE_HLSL;
	}

	const std::string parentPath{ options.m_FileName, pathInfo.m_FileName };
	if (parentPath.size())
		options.m_IncludePath.emplace_back(parentPath.c_str());

	const char* profile = nullptr;
	switch (options.m_Target)
	{
	case SLANG_DXIL:
		profile = "sm_6_7";
		if (!options.m_OutPath)
			options.m_Target = SLANG_DXIL_ASM;
		break;
	case SLANG_SPIRV:
	default:
		profile = "spirv_1_3";
		if (!options.m_OutPath)
			options.m_Target = SLANG_SPIRV_ASM;
		break;
	}

	auto& compiler = apollo::rdr::ShaderCompiler::s_Instance;
	std::span includePaths{ options.m_IncludePath.data(), options.m_IncludePath.size() };
	{
		std::span opts{ compilerOptions };
		const auto rc = compiler.Init(options.m_Target, profile, opts, includePaths);
		if (SLANG_FAILED(rc))
		{
			std::cerr << std::format("[0x{:08x}] Failed to initialize the compiler\n", rc);
			return 1;
		}
	}

	Slang::ComPtr source = LoadFile(options.m_FileName);
	if (!source->GetSize())
		return 1;

	Slang::ComPtr<slang::IBlob> diagnostics;
	slang::IModule* const module = compiler.LoadModuleFromSource(
		source,
		pathInfo.m_FileName,
		nullptr, // full path
		diagnostics.writeRef());
	if (!module)
	{
		std::cerr.write(
			static_cast<const char*>(diagnostics->getBufferPointer()),
			diagnostics->getBufferSize());
		return 1;
	}

	auto code = compiler.GetTargetCode(
		*module,
		options.m_EntryPointName,
		options.m_Stage,
		diagnostics.writeRef());
	if (!code)
	{
		std::cerr.write(
			static_cast<const char*>(diagnostics->getBufferPointer()),
			diagnostics->getBufferSize())
			<< '\n';
		return 1;
	}

	if (options.m_OutPath)
	{
		std::ofstream out{ options.m_OutPath };

		if (!out.is_open())
		{
			std::cerr << "Failed to open file " << options.m_OutPath << " for writing\n";
			return 1;
		}
		out.write(static_cast<const char*>(code->getBufferPointer()), code->getBufferSize());
	}
	else
	{
		std::cout.write(static_cast<const char*>(code->getBufferPointer()), code->getBufferSize());
	}

	return 0;
}