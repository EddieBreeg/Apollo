#pragma once

#include <core/Blob.hpp>
#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang.h>
#include <span>

namespace apollo::rdr {
	/**
	* \brief Unified API for shader management, using [Slang](https://shader-slang.org/)

	This is mostly used internally by the engine. A separate *shaderc* CMake target provides a
	command-line interface, typically useful to pre-compile shaders
	*/
	class ShaderCompiler
	{
	public:
		/**
		 * \brief Specialized \ref apollo::Blob "Blob" class which implements Slang's `IBlob`
		 * interface
		 */
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

		private:
			uint32 m_refCount = 0;
		};

		ShaderCompiler() = default;
		~ShaderCompiler() = default;
		ShaderCompiler(const ShaderCompiler&) = delete;
		ShaderCompiler(ShaderCompiler&&) = delete;
		ShaderCompiler& operator=(const ShaderCompiler&) = delete;
		ShaderCompiler& operator=(ShaderCompiler&&) = delete;

		/**
		 * \brief Initializes the compiler instance sessions
		 * \param targetFormat: The target format to compile to e.g. SPIRV, DXIL etc...
		 * \param profile: The capability profile to use
		 * \param compileOptions: The compiler options to use
		 * \param includePaths: A list of paths the compiler will look for modules in
		 */
		SlangResult Init(
			SlangCompileTarget targetFormat,
			const char* profile,
			std::span<const slang::CompilerOptionEntry> compileOptions,
			std::span<const char* const> includePaths);
		void Reset()
		{
			m_Session = nullptr;
			m_GlobalSession = nullptr;
		}

		/**
		 * \brief Loads a Slang module from a file, and option outputs a diagnostics message if an
		 * error occurs
		 * \param name: The path to the module
		 * \param out_diagnostics: If not `nullptr`, will be used to store a message if an error
		 * occurs
		 * \returns A pointer to the pre-compiled module, or `nullptr` on error
		 */
		slang::IModule* LoadModule(const char* name, slang::IBlob** out_diagnostics)
		{
			return m_Session->loadModule(name, out_diagnostics);
		}
		/**
		 * \brief Loads a Slang module from source directly.
		 * \param source: The source to compile from. See the \ref ShaderCompiler::Blob class for
		 * details.
		 * \param name: The module's name. This can be used to import it in other modules
		 * \param path: A path where the pre-compiled intermediate byte-code should be cached. May
		 * be `nullptr`, in which case the module will only be cached in memory
		 * \param out_diagnostics: If not `nullptr`, will be used to store a message if an error
		 * occurs
		 * \returns A pointer to the pre-compiled module, or `nullptr` on error
		 */
		slang::IModule* LoadModuleFromSource(
			slang::IBlob* source,
			const char* name,
			const char* path,
			slang::IBlob** out_diagnostics)
		{
			return m_Session->loadModuleFromSource(name, path, source, out_diagnostics);
		}

		/**
		 * \brief Links a module and returns the final binary
		 * \param module: The module to link
		 * \param entryPoint: The entry point to use for linking
		 * \param stage: The shader stage: vertex/fragment/compute or none
		 * \param out_diagnostics: If not `nullptr`, will be used to store a message if an error
		 * \details This function resolves all external dependencies, performs specialization for
		 * the provided entry point and returns the final binary, or `nullptr` if linking failed, in
		 * which case the diagnostics pointer will contain a message with more information (if
		 * provided). The \p stage parameter may be `SLANG_STAGE_NONE`, in which case the shader
		 * stage will not be checked and the entry point \b must be marked with the [[shader(...)]]
		 * attribute. If another value is used, marking the entry point is not necessary (although
		 * encouraged) and the shader stage will be validated.
		 */
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