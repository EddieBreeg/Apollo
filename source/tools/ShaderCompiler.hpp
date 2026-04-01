#pragma once

#include <slang-com-ptr.h>
#include <slang.h>
#include <span>
#include <system_error>

namespace apollo::rdr {
	class ShaderCompiler
	{
	public:
		ShaderCompiler() = default;
		~ShaderCompiler() = default;
		ShaderCompiler(const ShaderCompiler&) = delete;
		ShaderCompiler(ShaderCompiler&&) = delete;
		ShaderCompiler& operator=(const ShaderCompiler&) = delete;
		ShaderCompiler& operator=(ShaderCompiler&&) = delete;

		std::error_code Init(
			SlangCompileTarget targetFormat,
			const char* profile,
			std::span<const slang::CompilerOptionEntry> compileOptions,
			std::span<const char* const> includePaths);
		void Reset()
		{
			m_Session = nullptr;
			m_GlobalSession = nullptr;
		}

		slang::IModule* LoadModule(const char* name, slang::IBlob** out_diagnostics)
		{
			return m_Session->loadModule(name, out_diagnostics);
		}
		slang::IModule* LoadModuleFromSource(
			std::string_view source,
			const char* name,
			const char* path,
			slang::IBlob** out_diagnostics)
		{
			Slang::ComPtr sourceBlob{ slang_createBlob(source.data(), source.size()) };
			return m_Session->loadModuleFromSource(name, path, sourceBlob, out_diagnostics);
		}

		[[nodiscard]] Slang::ComPtr<slang::IBlob> GetTargetCode(
			slang::IModule& module,
			const char* entryPoint,
			SlangStage stage = SLANG_STAGE_NONE,
			slang::IBlob** diagnostics = nullptr);

		static ShaderCompiler s_Instance;

	private:
		Slang::ComPtr<slang::IGlobalSession> m_GlobalSession;
		Slang::ComPtr<slang::ISession> m_Session;
	};

	inline ShaderCompiler ShaderCompiler::s_Instance;
} // namespace apollo::rdr