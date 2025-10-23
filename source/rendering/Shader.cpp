#include "Shader.hpp"
#include "Context.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Enum.hpp>
#include <core/Log.hpp>

namespace {
	constexpr SDL_GPUShaderStage g_Stages[] = {
		SDL_GPU_SHADERSTAGE_VERTEX,
		SDL_GPU_SHADERSTAGE_FRAGMENT,
	};

#ifdef APOLLO_DEV
	const char* GetStageName(apollo::rdr::EShaderStage stage)
	{
		switch (stage)
		{
		case apollo::rdr::EShaderStage::Fragment: return "Fragment";
		case apollo::rdr::EShaderStage::Vertex: return "Vertex";
		default: return "Invalid";
		};
	}
#endif
} // namespace

#ifdef APOLLO_VULKAN
#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <spirv_reflect.h>

namespace {
	[[maybe_unused]] const char* GetSpirvReflectErrorMsg(SpvReflectResult code)
	{
		constexpr const char* msg[] = {
			"Success",
			"Not Ready",
			"Error Parse Failed",
			"Error Alloc Failed",
			"Error Range Exceeded",
			"Error Null Pointer",
			"Error Internal Error",
			"Error Count Mismatch",
			"Error Element Not Found",
			"Error Spirv Invalid Code Size",
			"Error Spirv Invalid Magic Number",
			"Error Spirv Unexpected Eof",
			"Error Spirv Invalid Id Reference",
			"Error Spirv Set Number Overflow",
			"Error Spirv Invalid Storage Class",
			"Error Spirv Recursion",
			"Error Spirv Invalid Instruction",
			"Error Spirv Unexpected Block Data",
			"Error Spirv Invalid Block Member Reference",
			"Error Spirv Invalid Entry Point",
			"Error Spirv Invalid Execution Mode",
			"Error Spirv Max Recursive Exceeded",
		};
		DEBUG_CHECK(code >= 0 && code < STATIC_ARRAY_SIZE(msg))
		{
			return "Unknown";
		}
		return msg[code];
	}

	struct ReflectionContext
	{
		SpvReflectShaderModule m_Module;

		bool Init(const void* code, size_t len)
		{
			const auto res = spvReflectCreateShaderModule(len, code, &m_Module);
			if (res != SPV_REFLECT_RESULT_SUCCESS)
			{
				APOLLO_LOG_ERROR("Failed to inspect SPIRV shader: {}", GetSpirvReflectErrorMsg(res));
				return false;
			}
			return true;
		}
		void GetInfo(apollo::rdr::ShaderInfo& out_info)
		{
			using namespace apollo::rdr;
			out_info.m_EntryPoint = m_Module.entry_point_name;
			switch (m_Module.shader_stage)
			{
			case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
				out_info.m_Stage = EShaderStage::Vertex;
				break;
			case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
				out_info.m_Stage = EShaderStage::Fragment;
				break;
			default:
				[[unlikely]] APOLLO_LOG_ERROR(
					"SPIRV shader reflection failed: invalid stage {}",
					uint32(m_Module.shader_stage));
				out_info.m_Stage = EShaderStage::Invalid;
				return;
			}

			for (uint32 i = 0; i < m_Module.descriptor_binding_count; ++i)
			{
				switch (m_Module.descriptor_bindings[i].descriptor_type)
				{
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					++out_info.m_NumStorageBuffers;
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					++out_info.m_NumUniformBuffers;
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: ++out_info.m_NumSamplers; break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					++out_info.m_NumStorageTextures;
					break;
				default: break;
				}
			}
		}

		~ReflectionContext() { spvReflectDestroyShaderModule(&m_Module); }
	};
} // namespace
#endif

namespace apollo::rdr {
	GraphicsShader::~GraphicsShader()
	{
		if (m_Handle)
			SDL_ReleaseGPUShader(Context::GetInstance()->GetDevice().GetHandle(), m_Handle);
	}

	GraphicsShader::GraphicsShader(
		const ULID& id,
		EShaderStage stage,
		const void* code,
		size_t codeLen)
		: IAsset(id)
	{
		ReflectionContext ctx;
		if (!ctx.Init(code, codeLen))
			return;
		ShaderInfo info{};
		ctx.GetInfo(info);
		if (info.m_Stage != stage)
		{
			APOLLO_LOG_ERROR(
				"Failed to create shader from byte code: expected stage {}, got {}",
				GetStageName(stage),
				GetStageName(info.m_Stage));
		}

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
		GPUDevice& device = Context::GetInstance()->GetDevice();
		m_Handle = SDL_CreateGPUShader(device.GetHandle(), &createInfo);
		if (!m_Handle) [[unlikely]]
		{
			APOLLO_LOG_ERROR("Failed to create shader: {}", SDL_GetError());
		}
	}

	GraphicsShader::GraphicsShader(
		const ULID& id,
		EShaderStage stage,
		std::string_view hlsl,
		const char* entryPoint)
		: IAsset(id)
	{
#ifdef APOLLO_VULKAN
		constexpr EShLanguage vkStages[] = {
			EShLanguage::EShLangVertex,
			EShLanguage::EShLangFragment,
		};
		const EShLanguage vkStage = vkStages[ToUnderlying(stage)];
		glslang::TShader vkShader{ vkStage };
		const char* tempPtr = hlsl.data();
		const int32 tempLen = (int32)hlsl.length();
		vkShader.setStringsWithLengths(&tempPtr, &tempLen, 1);
		vkShader.setEntryPoint(entryPoint);
		vkShader.setEnvInput(glslang::EShSourceHlsl, vkStage, glslang::EShClientVulkan, 100);
		vkShader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
		vkShader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
		if (!vkShader.parse(GetDefaultResources(), 100, true, EShMsgDefault))
		{
			APOLLO_LOG_ERROR("Shader compilation failed: {}", vkShader.getInfoLog());
			return;
		}
		glslang::TProgram vkProg;
		vkProg.addShader(&vkShader);
		if (!vkProg.link(EShMsgDefault))
		{
			APOLLO_LOG_ERROR("Shader linking failed: {}", vkProg.getInfoLog());
			return;
		}
		std::vector<uint32> code;
		spv::SpvBuildLogger logger;
		glslang::SpvOptions options{};
#ifdef APOLLO_DEV
		options.generateDebugInfo = true;
#else
		options.disableOptimizer = false;
#endif
		glslang::GlslangToSpv(*vkProg.getIntermediate(vkStage), code, &logger, &options);
		std::string log = logger.getAllMessages();
		if (log.size())
		{
			APOLLO_LOG_ERROR("SPIRV Generation failed: {}", log);
			return;
		}
		const size_t codeLen = 4 * code.size();
#endif
		ReflectionContext ctx{};
		if (!ctx.Init(code.data(), codeLen))
			return;

		ShaderInfo info{};
		ctx.GetInfo(info);

		SDL_GPUShaderCreateInfo createInfo{
			.code_size = codeLen,
			.code = (uint8*)code.data(),
			.entrypoint = entryPoint,
			.format = SDL_GPU_SHADERFORMAT_SPIRV,
			.stage = g_Stages[ToUnderlying(stage)],
			.num_samplers = info.m_NumSamplers,
			.num_storage_textures = info.m_NumStorageTextures,
			.num_storage_buffers = info.m_NumStorageBuffers,
			.num_uniform_buffers = info.m_NumUniformBuffers,
		};
		GPUDevice& device = Context::GetInstance()->GetDevice();
		m_Handle = SDL_CreateGPUShader(device.GetHandle(), &createInfo);
		if (!m_Handle) [[unlikely]]
		{
			APOLLO_LOG_ERROR("Failed to create shader: {}", SDL_GetError());
		}
	}
} // namespace apollo::rdr