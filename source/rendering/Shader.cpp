#include "Shader.hpp"
#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Enum.hpp>
#include <core/Log.hpp>

namespace {
	constexpr SDL_GPUShaderStage g_Stages[] = {
		SDL_GPU_SHADERSTAGE_VERTEX,
		SDL_GPU_SHADERSTAGE_FRAGMENT,
	};
}

namespace apollo::rdr {
	Shader::~Shader()
	{
		if (m_Handle)
			SDL_ReleaseGPUShader(Renderer::GetInstance()->GetDevice().GetHandle(), m_Handle);
	}

	Shader::Shader(const ULID& id, const ShaderInfo& info, const void* code, size_t codeLen)
		: IAsset(id)
		, m_Stage(info.m_Stage)
	{
		APOLLO_ASSERT(
			m_Stage > EShaderStage::Invalid && info.m_Stage < EShaderStage::NStages,
			"Invalid shader stage {}",
			int32(info.m_Stage));

		const SDL_GPUShaderCreateInfo createInfo{
			.code_size = codeLen,
			.code = static_cast<const uint8*>(code),
			.entrypoint = info.m_EntryPoint,
#ifdef APOLLO_VULKAN
			.format = SDL_GPU_SHADERFORMAT_SPIRV,
#endif
			.stage = g_Stages[int32(info.m_Stage)],
			.num_samplers = info.m_NumSamplers,
			.num_storage_textures = info.m_NumStorageTextures,
			.num_storage_buffers = info.m_NumStorageBuffers,
			.num_uniform_buffers = info.m_NumUniformBuffers,
		};
		GPUDevice& device = Renderer::GetInstance()->GetDevice();
		m_Handle = SDL_CreateGPUShader(device.GetHandle(), &createInfo);
		if (!m_Handle) [[unlikely]]
		{
			APOLLO_LOG_ERROR("Failed to create shader: {}", SDL_GetError());
		}
	}

	Shader::Shader(const ShaderInfo& info, const void* code, size_t codeLen)
		: Shader(ULID::Generate(), info, code, codeLen)
	{}
} // namespace apollo::rdr