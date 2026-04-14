#include "Shader.hpp"
#include "Context.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Enum.hpp>
#include <core/Log.hpp>
#include <core/NumConv.hpp>
#include <tools/ShaderCompiler.hpp>

namespace {
	constexpr SDL_GPUShaderStage g_SdlStages[] = {
		SDL_GPU_SHADERSTAGE_VERTEX,
		SDL_GPU_SHADERSTAGE_FRAGMENT,
	};
	constexpr SlangStage g_SlangStages[] = {
		SLANG_STAGE_VERTEX,
		SLANG_STAGE_FRAGMENT,
	};
} // namespace

namespace {
	SDL_GPUShader* CreateShader(
		slang::IBlob* code,
		const char* entryPoint,
		const apollo::rdr::ShaderInfo& info,
		SDL_GPUDevice& device)
	{
		SDL_GPUShaderCreateInfo createInfo{
			.code_size = code->getBufferSize(),
			.code = static_cast<const uint8*>(code->getBufferPointer()),
			.entrypoint = entryPoint,
			.format = SDL_GPU_SHADERFORMAT_SPIRV,
			.stage = g_SdlStages[apollo::ToUnderlying(info.m_Stage)],
			.num_samplers = info.m_NumSamplers,
			.num_storage_textures = info.m_NumStorageTextures,
			.num_storage_buffers = info.m_NumStorageBuffers,
			.num_uniform_buffers = info.m_NumUniformBuffers,
		};
		return SDL_CreateGPUShader(&device, &createInfo);
	}

	SDL_GPUShader* CreateShader(
		slang::IModule& module,
		[[maybe_unused]] const apollo::ULID& id,
		apollo::rdr::ShaderInfo& out_info,
		apollo::rdr::EShaderStage stage,
		const char* entryPoint,
		apollo::rdr::ShaderCompiler& compiler,
		SDL_GPUDevice& device)
	{
		Slang::ComPtr<slang::IBlob> diagnostics;
		auto const program = compiler.ComposeAndLink(
			module,
			entryPoint,
			g_SlangStages[apollo::ToUnderlying(stage)],
			diagnostics.writeRef());
		if (!program)
		{
#ifdef APOLLO_DEV
			char name[27] = {};
			id.ToChars(name);
			std::string_view msg{
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize(),
			};
			APOLLO_LOG_ERROR("Linking failed for shader {}:\n{}", name, msg);
#endif
			return nullptr;
		}
		Slang::ComPtr<slang::IBlob> code;
		program->getTargetCode(0, code.writeRef());

		Slang::ComPtr<slang::IMetadata> metadata;
		program->getEntryPointMetadata(0, 0, metadata.writeRef(), diagnostics.writeRef());
		if (!metadata) [[unlikely]]
		{
#ifdef APOLLO_DEV
			char name[27] = {};
			id.ToChars(name);
			APOLLO_LOG_ERROR(
				"Failed to get metadata for shader {}:\n{:.{}}",
				name,
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize());
#endif
			return nullptr;
		}
		out_info = apollo::rdr::ShaderInfo::FromSlangModule(*program, stage, metadata);
		return CreateShader(code, entryPoint, out_info, device);
	}
} // namespace

namespace apollo::rdr {
	GraphicsShader::~GraphicsShader()
	{
		if (m_Handle)
			SDL_ReleaseGPUShader(Context::GetInstance()->GetDevice().GetHandle(), m_Handle);
	}

	GraphicsShader::GraphicsShader(
		const ULID& id,
		EShaderStage stage,
		slang::IModule& module,
		const char* entryPoint)
		: IAsset(id)
	{
		GPUDevice& device = Context::GetInstance()->GetDevice();
		m_Handle = CreateShader(
			module,
			id,
			m_Info,
			stage,
			entryPoint,
			apollo::rdr::ShaderCompiler::s_Instance,
			*device.GetHandle());
		if (!m_Handle) [[unlikely]]
		{
			APOLLO_LOG_ERROR("Failed to create shader: {}", SDL_GetError());
		}
	}

	GraphicsShader::GraphicsShader(
		const ULID& id,
		EShaderStage stage,
		ISlangBlob* source,
		const char* entryPoint)
		: IAsset(id)
	{
		char name[27] = {};
		id.ToChars(name);
		auto& compiler = ShaderCompiler::s_Instance;
		Slang::ComPtr<slang::IBlob> diagnostics;
		slang::IModule* const module = compiler.LoadModuleFromSource(
			source,
			name,
			nullptr,
			diagnostics.writeRef());
		if (!module)
		{
			std::string_view msg{
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize(),
			};
			APOLLO_LOG_ERROR("Compilation failed for shader {}:\n{}", name, msg);
			return;
		}
		GPUDevice& device = Context::GetInstance()->GetDevice();
		m_Handle = CreateShader(
			*module,
			id,
			m_Info,
			stage,
			entryPoint,
			rdr::ShaderCompiler::s_Instance,
			*device.GetHandle());
		if (!m_Handle) [[unlikely]]
		{
			APOLLO_LOG_ERROR("Failed to create shader: {}", SDL_GetError());
		}
	}
	GraphicsShader::GraphicsShader(
		[[maybe_unused]] const ULID& id,
		EShaderStage stage,
		slang::IModule& module,
		slang::IEntryPoint& entryPoint)
	{
		auto& compiler = ShaderCompiler::s_Instance;

		Slang::ComPtr<slang::IBlob> diagnostics;
		auto const program = compiler.ComposeAndLink(module, entryPoint, diagnostics.writeRef());
		if (!program)
		{
#ifdef APOLLO_DEV
			std::string_view msg{
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize(),
			};
			char name[27] = {};
			id.ToChars(name);
			APOLLO_LOG_ERROR("Linking failed for shader {}:\n{}", name, msg);
#endif
			return;
		}
		Slang::ComPtr<slang::IBlob> code;
		program->getTargetCode(0, code.writeRef());

		Slang::ComPtr<slang::IMetadata> metadata;
		program->getEntryPointMetadata(0, 0, metadata.writeRef(), diagnostics.writeRef());
		if (!metadata) [[unlikely]]
		{
#ifdef APOLLO_DEV
			char name[27] = {};
			id.ToChars(name);
			APOLLO_LOG_ERROR(
				"Failed to get metadata for shader {}:\n{:.{}}",
				name,
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize());
#endif
			return;
		}
		m_Info = apollo::rdr::ShaderInfo::FromSlangModule(*program, stage, metadata);
		m_Handle = CreateShader(
			code,
			entryPoint.getFunctionReflection()->getName(),
			m_Info,
			*Context::GetInstance()->GetDevice().GetHandle());
	}
} // namespace apollo::rdr