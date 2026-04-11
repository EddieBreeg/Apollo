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
} // namespace

namespace apollo::rdr {
	SlangResult ShaderCompiler::Init(
		SlangCompileTarget targetFormat,
		const char* profile,
		std::span<const slang::CompilerOptionEntry> compileOptions,
		std::span<const char* const> includePaths)
	{
		if (m_GlobalSession) [[unlikely]]
		{
			LOG(spdlog::level::warn,
				"Called ShaderCompiler::Init() but compiler is already initialized");
			return SLANG_OK;
		}
		auto res = slang::createGlobalSession(m_GlobalSession.writeRef());
		if (SLANG_FAILED(res)) [[unlikely]]
			return res;

		if (targetFormat == SLANG_TARGET_NONE)
		{
			const slang::SessionDesc desc{
				.targets = nullptr,
				.targetCount = 0,
				.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
				.searchPaths = includePaths.data(),
				.searchPathCount = static_cast<long long>(includePaths.size()),
				.skipSPIRVValidation = false,
			};
			res = m_GlobalSession->createSession(desc, m_Session.writeRef());
			return res;
		}

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
		return res;
	}

	Slang::ComPtr<slang::IComponentType> ShaderCompiler::ComposeAndLink(
		slang::IModule& module,
		const char* entryPoint,
		SlangStage stage,
		slang::IBlob** diagnostics)
	{
		Slang::ComPtr ep = stage != SLANG_STAGE_NONE
							   ? GetEntryPointFromNameAndStage(
									 module,
									 entryPoint,
									 stage,
									 diagnostics)
							   : GetEntryPointFromName(module, entryPoint, diagnostics);
		return ep ? ComposeAndLink(module, *ep, diagnostics) : nullptr;
	}

	Slang::ComPtr<slang::IComponentType> ShaderCompiler::ComposeAndLink(
		slang::IModule& module,
		slang::IEntryPoint& entryPoint,
		slang::IBlob** out_diagnostics)
	{
		Slang::ComPtr<slang::IComponentType> linked, composite;
		slang::IComponentType* const components[] = {
			&module,
			&entryPoint,
		};
		m_Session->createCompositeComponentType(
			components,
			2,
			composite.writeRef(),
			out_diagnostics);
		if (!composite)
			return linked;
		composite->link(linked.writeRef(), out_diagnostics);
		return linked;
	}

	Slang::ComPtr<slang::IBlob> ShaderCompiler::GetTargetCode(
		slang::IModule& module,
		const char* entryPoint,
		SlangStage stage,
		slang::IBlob** diagnostics)
	{
		auto const prog = ComposeAndLink(module, entryPoint, stage, diagnostics);
		Slang::ComPtr<slang::IBlob> code;
		if (prog)
		{
			prog->getTargetCode(0, code.writeRef(), diagnostics);
		}
		return code;
	}
} // namespace apollo::rdr