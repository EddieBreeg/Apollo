#include "Command.hpp"
#include "Buffer.hpp"
#include "Context.hpp"
#include "Material.hpp"
#include "RenderPass.hpp"
#include "backends/imgui_impl_sdlgpu3.h"
#include <SDL3/SDL_gpu.h>
#include <array>
#include <core/Assert.hpp>
#include <core/NumConv.hpp>

namespace apollo::rdr {
#define COMMAND_TYPE_IMPL(type)                                                                    \
	template <>                                                                                    \
	void GPUCommand::Impl<GPUCommand::ECommandType::type>(Context & ctx)

	COMMAND_TYPE_IMPL(PushVertexShaderConstants)
	{
		SDL_PushGPUVertexUniformData(
			ctx.GetMainCommandBuffer(),
			m_Storage.m_Constants->m_Slot,
			m_Storage.m_Constants->m_Buffer,
			m_Storage.m_Constants->m_Size);
	}

	COMMAND_TYPE_IMPL(PushFragmentShaderConstants)
	{
		SDL_PushGPUFragmentUniformData(
			ctx.GetMainCommandBuffer(),
			m_Storage.m_Constants->m_Slot,
			m_Storage.m_Constants->m_Buffer,
			m_Storage.m_Constants->m_Size);
	}

	COMMAND_TYPE_IMPL(BeginRenderPass)
	{
		ctx.SwitchRenderPass(m_Storage.m_RenderPass);
	}

	COMMAND_TYPE_IMPL(SetViewport)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		const SDL_GPUViewport vp{
			.x = m_Storage.m_Viewport.x0,
			.y = m_Storage.m_Viewport.y0,
			.w = m_Storage.m_Viewport.GetWidth(),
			.h = m_Storage.m_Viewport.GetHeight(),
			.min_depth = 0.0f,
			.max_depth = 1.0f,
		};
		SDL_SetGPUViewport(renderPass->GetHandle(), &vp);
	}
	COMMAND_TYPE_IMPL(BindGraphicsPipeline)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		SDL_BindGPUGraphicsPipeline(renderPass->GetHandle(), m_Storage.m_Pipeline);
	}

	COMMAND_TYPE_IMPL(BindMaterialInstance)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		m_Storage.m_MaterialInstance->PushFragmentConstants(ctx.GetMainCommandBuffer());
		m_Storage.m_MaterialInstance->Bind(renderPass->GetHandle());
	}

	COMMAND_TYPE_IMPL(BindVertexBuffers)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");

		auto* bindings = Alloca(SDL_GPUBufferBinding, m_Storage.m_Buffers.size());
		for (size_t i = 0; i < m_Storage.m_Buffers.size(); ++i)
		{
			bindings[i] = {
				.buffer = m_Storage.m_Buffers[i].GetHandle(),
			};
		}
		SDL_BindGPUVertexBuffers(
			renderPass->GetHandle(),
			0,
			bindings,
			NumCast<uint32>(m_Storage.m_Buffers.size()));
	}

	COMMAND_TYPE_IMPL(BindIndexBuffer)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		const SDL_GPUBufferBinding binding{
			.buffer = m_Storage.m_IBuffer->GetHandle(),
		};
		SDL_BindGPUIndexBuffer(renderPass->GetHandle(), &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
	}

	COMMAND_TYPE_IMPL(BindVertexStorageBuffers)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");

		auto* bindings = Alloca(SDL_GPUBuffer*, m_Storage.m_Buffers.size());
		for (size_t i = 0; i < m_Storage.m_Buffers.size(); ++i)
		{
			bindings[i] = m_Storage.m_Buffers[i].GetHandle();
		}

		SDL_BindGPUVertexStorageBuffers(
			renderPass->GetHandle(),
			0,
			bindings,
			NumCast<uint32>(m_Storage.m_Buffers.size()));
	}

	COMMAND_TYPE_IMPL(BindFragmentStorageBuffers)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");

		auto* bindings = Alloca(SDL_GPUBuffer*, m_Storage.m_Buffers.size());
		for (size_t i = 0; i < m_Storage.m_Buffers.size(); ++i)
		{
			bindings[i] = m_Storage.m_Buffers[i].GetHandle();
		}

		SDL_BindGPUFragmentStorageBuffers(
			renderPass->GetHandle(),
			0,
			bindings,
			NumCast<uint32>(m_Storage.m_Buffers.size()));
	}

	COMMAND_TYPE_IMPL(DrawPrimitives)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		SDL_DrawGPUPrimitives(
			renderPass->GetHandle(),
			m_Storage.m_DrawCall.m_NumVertices,
			m_Storage.m_DrawCall.m_NumInstances,
			m_Storage.m_DrawCall.m_FistVertex,
			m_Storage.m_DrawCall.m_FistInstance);
	}

	COMMAND_TYPE_IMPL(DrawIndexedPrimitives)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		SDL_DrawGPUIndexedPrimitives(
			renderPass->GetHandle(),
			m_Storage.m_IndexedDrawCall.m_NumIndices,
			m_Storage.m_IndexedDrawCall.m_NumInstances,
			m_Storage.m_IndexedDrawCall.m_FirstIndex,
			m_Storage.m_IndexedDrawCall.m_VertexOffset,
			m_Storage.m_IndexedDrawCall.m_FirstInstance);
	}

	COMMAND_TYPE_IMPL(DrawImGuiLayer)
	{
		auto* const cmdBuffer = ctx.GetMainCommandBuffer();
		auto* const swapChainTexture = ctx.GetSwapchainTexture();

		if (!swapChainTexture || !m_Storage.m_ImGuiDrawCall.m_DrawData) [[unlikely]]
			return;

		ctx.SwitchRenderPass();

		ImGui_ImplSDLGPU3_PrepareDrawData(m_Storage.m_ImGuiDrawCall.m_DrawData, cmdBuffer);
		const SDL_GPUColorTargetInfo targetInfo{
			.texture = swapChainTexture,
			.clear_color =
				SDL_FColor{
					m_Storage.m_ImGuiDrawCall.m_ClearColor.r,
					m_Storage.m_ImGuiDrawCall.m_ClearColor.g,
					m_Storage.m_ImGuiDrawCall.m_ClearColor.b,
					m_Storage.m_ImGuiDrawCall.m_ClearColor.a,
				},
			.load_op = m_Storage.m_ImGuiDrawCall.m_ClearTarget ? SDL_GPU_LOADOP_CLEAR
															   : SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_STORE,
		};

		SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmdBuffer, &targetInfo, 1, nullptr);
		ImGui_ImplSDLGPU3_RenderDrawData(m_Storage.m_ImGuiDrawCall.m_DrawData, cmdBuffer, pass);
		SDL_EndGPURenderPass(pass);
	}

	COMMAND_TYPE_IMPL(Custom)
	{
		m_Storage.m_Custom.m_Invoke(m_Storage.m_Custom.m_Buf, ctx);
	}

#undef COMMAND_TYPE_IMPL

	GPUCommand::GPUCommand(EShaderStage stage, const void* data, size_t size, uint32 slot)
		: m_Storage{ ._unused = 0 }
	{
		APOLLO_ASSERT(
			size <= sizeof(ShaderConstantStorage),
			"Data is too large for shader data constant block: {} bytes",
			size);
		APOLLO_ASSERT(slot < 4, "Shader constant block index {} is out of range", slot);

		const size_t allocSize = sizeof(ShaderConstantCommand) + size - 1;
		m_Storage.m_Constants = static_cast<ShaderConstantCommand*>(::operator new(allocSize));
		new (m_Storage.m_Constants) ShaderConstantCommand{
			.m_Size = static_cast<uint32>(size),
			.m_Slot = slot,
		};
		memcpy(m_Storage.m_Constants->m_Buffer, data, size);
		switch (stage)
		{
		case EShaderStage::Vertex: m_Type = PushVertexShaderConstants; break;
		case EShaderStage::Fragment: m_Type = PushFragmentShaderConstants; break;
		default:
			[[unlikely]] APOLLO_LOG_CRITICAL("Invalid shader stage {}", int32(stage));
			DEBUG_BREAK();
		}
	}

	GPUCommand::GPUCommand(ECommandType t, const Buffer& buf)
		: m_Storage{ ._unused = 0 }
		, m_Type(t)
	{
		APOLLO_ASSERT(buf, "Passed null buffer to GPU command");
		switch (t)
		{
		case ECommandType::BindIndexBuffer:
			APOLLO_ASSERT(
				buf.IsIndexBuffer(),
				"Command type is BindIndexBuffer but buffer is not an index buffer");
			m_Storage.m_IBuffer = &buf;
			return;
#ifdef APOLLO_DEV
		case ECommandType::BindVertexBuffers:
			APOLLO_ASSERT(
				buf.IsVertexBuffer(),
				"Command type is BindVertexBuffers but buffer is not a vertex buffer");
			break;
		case ECommandType::BindVertexStorageBuffers:
		case ECommandType::BindFragmentStorageBuffers:
			APOLLO_ASSERT(
				buf.GetUsageFlags().HasAny(EBufferFlags::GraphicsStorage),
				"buffer doesn't have the GraphicsStorage flag");
			break;
		default:
			APOLLO_LOG_CRITICAL("Command type {} is not a buffer bind command", uint32(t));
			DEBUG_BREAK();
#else
		default: break;
#endif
		}

		m_Storage.m_Buffers = std::span{ &buf, 1 };
	}

	GPUCommand::GPUCommand(ECommandType type, std::span<const Buffer> buffers)
		: m_Storage{ .m_Buffers{ buffers } }
		, m_Type(type)
	{
		if (buffers.empty()) [[unlikely]]
			return;

#ifdef APOLLO_DEV
		EBufferFlags expectedFlags;
		switch (type)
		{
		case ECommandType::BindVertexBuffers: expectedFlags = EBufferFlags::Vertex; break;
		case ECommandType::BindVertexStorageBuffers: [[fallthrough]];
		case ECommandType::BindFragmentStorageBuffers:
			expectedFlags = EBufferFlags::GraphicsStorage;
			break;
		default:
			APOLLO_LOG_CRITICAL(
				"Passed buffer list to GPUCommand constructor with command type {}",
				uint32(type));
			DEBUG_BREAK();
		}
		for (const Buffer& b : buffers)
		{
			APOLLO_ASSERT(
				b.GetUsageFlags().HasAll(expectedFlags),
				"Passed GPU buffers with conflicting usage flags to GPUCommand constructor");
		}
#endif
	}

	GPUCommand::GPUCommand(GPUCommand&& other) noexcept
		: m_Storage(other.m_Storage)
		, m_Type(other.m_Type)
	{}

	GPUCommand& GPUCommand::operator=(GPUCommand&& other) noexcept
	{
		this->~GPUCommand();
		m_Storage = other.m_Storage;
		m_Type = other.m_Type;
		other.m_Type = ECommandType::Invalid;
		return *this;
	}

	GPUCommand::~GPUCommand()
	{
		if (m_Type == ECommandType::PushVertexShaderConstants ||
			m_Type == ECommandType::PushFragmentShaderConstants)
		{
			::operator delete(m_Storage.m_Constants);
		}
	}

	void GPUCommand::operator()(Context& ctx)
	{
		APOLLO_ASSERT(m_Type != ECommandType::Invalid, "Attempting to invoke invalid GPU command");
		static constexpr std::array impl = []<size_t... Types>(std::index_sequence<Types...>)
		{
			using ArrayType = std::array<void (GPUCommand::*)(Context&), sizeof...(Types)>;
			return ArrayType{ &GPUCommand::Impl<static_cast<ECommandType>(Types)>... };
		}(std::make_index_sequence<size_t(ECommandType::NTypes)>());

		return (this->*impl[uint8(m_Type)])(ctx);
	}
} // namespace apollo::rdr