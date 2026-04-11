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

#ifdef APOLLO_VULKAN
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
				if (!m_Module.descriptor_bindings[i].accessed)
					continue;

				switch (m_Module.descriptor_bindings[i].descriptor_type)
				{
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					++out_info.m_NumStorageBuffers;
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					if (out_info.m_NumUniformBuffers >= STATIC_ARRAY_SIZE(out_info.m_Blocks))
						[[unlikely]]
					{
						APOLLO_LOG_ERROR(
							"Shader uniform buffer count exceeds the maximum allowed ({})",
							STATIC_ARRAY_SIZE(out_info.m_Blocks));
					}
					else
					{
						AddUniformBlock(
							m_Module.descriptor_bindings[i].block,
							out_info.m_Blocks[out_info.m_NumUniformBuffers++]);
					}
					break;

				case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: ++out_info.m_NumSamplers; break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					++out_info.m_NumStorageTextures;
					break;
				default: break;
				}
			}
		}

		void AddUniformBlock(
			const SpvReflectBlockVariable& block,
			apollo::rdr::ShaderConstantBlock& out_block)
		{
			using namespace apollo::rdr;

			out_block.m_Name = block.name;
			out_block.m_Size = block.padded_size;
			out_block.m_NumMembers = block.member_count;
			out_block.m_Members = std::make_unique<ShaderConstant[]>(out_block.m_NumMembers);
			for (uint32 i = 0; i < out_block.m_NumMembers; ++i)
			{
				auto& member = block.members[i];
				out_block.m_Members[i].m_Name = member.name;
				out_block.m_Members[i].m_Offset = member.offset;
				out_block.m_Members[i].m_Type = ParseParamType(*member.type_description);
			}
		}

		apollo::rdr::ShaderConstant::EType ParseParamType(const SpvReflectTypeDescription& desc)
		{
			using apollo::rdr::ShaderConstant;
			ShaderConstant::EType type = ShaderConstant::Invalid;

			if (desc.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
			{
				type = ShaderConstant::Float;
			}
			else if (desc.type_flags & SPV_REFLECT_TYPE_FLAG_INT)
			{
				type = desc.traits.numeric.scalar.signedness ? ShaderConstant::Int
															 : ShaderConstant::UInt;
			}
			else
			{
				return type;
			}

			if (!(desc.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR))
				return type;

			const uint32 comp = desc.traits.numeric.vector.component_count - 1;
			type = ShaderConstant::EType(type + comp);
			if (desc.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
			{
				const uint32 r = desc.traits.numeric.matrix.row_count - 1;
				type = ShaderConstant::EType(type + 3 * r);
			}
			return type;
		}

		~ReflectionContext() { spvReflectDestroyShaderModule(&m_Module); }
	};
} // namespace
#endif

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
		const apollo::ULID& id,
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
		const ULID& id,
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