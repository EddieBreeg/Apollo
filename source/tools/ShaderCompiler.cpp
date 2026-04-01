#include "ShaderCompiler.hpp"
#include <spdlog/spdlog.h>

#if APOLLO_DEV
#define LOG(level, ...)                                                                            \
	spdlog::log(spdlog::source_loc{ __FILE__, __LINE__, __func__ }, level, __VA_ARGS__)
#else
#define LOG(level, ...) (void)level
#endif

namespace {
	Slang::ComPtr<slang::IEntryPoint> GetEntryPointFromName(
		slang::IModule& module,
		const char* entryPointName,
		slang::IBlob** diagnostics)
	{
		Slang::ComPtr<slang::IEntryPoint> ep;
		const auto err = module.findEntryPointByName(entryPointName, ep.writeRef());
		if (SLANG_FAILED(err) && diagnostics)
		{
			const std::string msg = fmt::format("Entry point '{}' was not found", entryPointName);
			*diagnostics = slang_createBlob(msg.c_str(), msg.size());
		}
		return ep;
	}
	Slang::ComPtr<slang::IEntryPoint> GetEntryPointFromNameAndStage(
		slang::IModule& module,
		const char* name,
		SlangStage stage,
		slang::IBlob** diagnostics)
	{
		Slang::ComPtr<slang::IEntryPoint> ep;
		module.findAndCheckEntryPoint(name, stage, ep.writeRef(), diagnostics);
		return ep;
	}

	Slang::ComPtr<slang::IBlob> GetModuleCode(
		slang::ISession& session,
		slang::IModule& module,
		slang::IEntryPoint& entryPoint,
		slang::IBlob** diagnostics)
	{
		using Slang::ComPtr;
		Slang::ComPtr<slang::IBlob> code;

		slang::IComponentType* const comp[]{ &module, &entryPoint };
		ComPtr<slang::IComponentType> composed;
		session.createCompositeComponentType(comp, 2, composed.writeRef(), diagnostics);
		if (!composed)
			return code;

		ComPtr<slang::IComponentType> program;

		composed->link(program.writeRef(), diagnostics);
		if (!program)
			return code;

		program->getTargetCode(0, code.writeRef(), diagnostics);
		return code;
	}
} // namespace

namespace apollo::rdr {
	std::error_code ShaderCompiler::Init(
		SlangCompileTarget targetFormat,
		const char* profile,
		std::span<const slang::CompilerOptionEntry> compileOptions,
		std::span<const char* const> includePaths)
	{
		if (m_GlobalSession) [[unlikely]]
		{
			LOG(spdlog::level::warn,
				"Called ShaderCompiler::Init() but compiler is already initialized");
			return std::error_code{};
		}
		auto res = slang::createGlobalSession(m_GlobalSession.writeRef());
		if (SLANG_FAILED(res)) [[unlikely]]
			return { res, std::system_category() };

		const slang::TargetDesc targetDesc{
			.format = targetFormat,
			.profile = m_GlobalSession->findProfile(profile),
			.flags = kDefaultTargetFlags,
			.compilerOptionEntries = compileOptions.data(),
			.compilerOptionEntryCount = static_cast<uint32_t>(compileOptions.size()),
		};

		const slang::SessionDesc desc{
			.targets = &targetDesc,
			.targetCount = 1,
			.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
			.searchPaths = includePaths.data(),
			.searchPathCount = static_cast<long long>(includePaths.size()),
			.skipSPIRVValidation = false,
		};
		res = m_GlobalSession->createSession(desc, m_Session.writeRef());
		return { res, std::system_category() };
	}

	Slang::ComPtr<slang::IBlob> ShaderCompiler::GetTargetCode(
		slang::IModule& module,
		const char* entryPoint,
		SlangStage stage,
		slang::IBlob** diagnostics)
	{
		using Slang::ComPtr;

		Slang::ComPtr<slang::IBlob> res;
		ComPtr ep = stage != SLANG_STAGE_NONE
						? GetEntryPointFromNameAndStage(module, entryPoint, stage, diagnostics)
						: GetEntryPointFromName(module, entryPoint, diagnostics);
		if (!ep)
			return res;

		return GetModuleCode(*m_Session, module, *ep, diagnostics);
	}
} // namespace apollo::rdr