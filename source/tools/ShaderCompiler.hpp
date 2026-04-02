#pragma once

#include <core/Blob.hpp>
#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang.h>
#include <span>
#include <system_error>

namespace apollo::rdr {
	class ShaderCompiler
	{
	public:
		struct Blob final : public apollo::Blob, slang::IBlob
		{
			Blob(void* ptr, size_t n)
				: apollo::Blob(ptr, n)
			{}

			ISlangUnknown* getInterface(const SlangUUID& id) noexcept
			{
				return id == ISlangBlob::getTypeGuid() ? this : nullptr;
			}

			[[nodiscard("Discarding the result would cause a leak")]] static Blob* Allocate(size_t n)
			{
				return apollo::Blob::Allocate<ShaderCompiler::Blob>(n);
			}
			[[nodiscard("Discarding the result would cause a leak")]] static Blob* Allocate(
				size_t n,
				const void* data)
			{
				Blob* blob = apollo::Blob::Allocate<ShaderCompiler::Blob>(n);
				memcpy(blob->GetPtr(), data, n);
				return blob;
			}

			SLANG_IUNKNOWN_ALL

			void const* SLANG_MCALL getBufferPointer() noexcept override { return m_Ptr; }
			size_t SLANG_MCALL getBufferSize() noexcept override { return m_Size; }

			uint32 m_refCount = 0;
		};

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
			slang::IBlob* source,
			const char* name,
			const char* path,
			slang::IBlob** out_diagnostics)
		{
			return m_Session->loadModuleFromSource(name, path, source, out_diagnostics);
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