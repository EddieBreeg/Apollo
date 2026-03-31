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

	auto path = std::filesystem::path(options.m_FileName).parent_path().string();
	if (path.size())
		options.m_IncludePath.emplace_back(path.c_str());

	const char* profile = nullptr;
	const char* ext = nullptr;
	switch (options.m_Target)
	{
	case SLANG_DXIL:
		profile = "sm_6_7";
		ext = "dxil";
		break;
	case SLANG_SPIRV:
	default:
		profile = "spirv_6";
		ext = "spv";
		break;
	}

	auto& compiler = apollo::rdr::ShaderCompiler::s_Instance;
	std::span includePaths{ options.m_IncludePath.data(), options.m_IncludePath.size() };
	if (const auto rc = compiler.Init(options.m_Target, profile, {}, includePaths); rc)
	{
		std::cerr << "Failed to initialize the compiler: " << rc.message() << '\n';
		return 1;
	}

	Slang::ComPtr<slang::IBlob> diagnostics;
	auto code = compiler.Compile(
		options.m_FileName,
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

	const std::string outPath{ options.m_OutPath ? options.m_OutPath
												 : std::format("{}.{}", options.m_FileName, ext) };
	std::ofstream out{ outPath };

	if (!out.is_open())
	{
		std::cerr << "Failed to open file " << outPath << " for writing\n";
		return 1;
	}
	out.write(static_cast<const char*>(code->getBufferPointer()), code->getBufferSize());

	return 0;
}