#include "Command.hpp"
#include "Buffer.hpp"
#include "Context.hpp"
#include "Material.hpp"
#include "RenderPass.hpp"
#include <SDL3/SDL_gpu.h>
#include <array>
#include <core/Assert.hpp>
#include <core/NumConv.hpp>

namespace apollo::rdr {
#define COMMAND_TYPE_IMPL(type)                                                                    \
	template <>                                                                                    \
	void GPUCommand::Impl<GPUCommand::ECommandType::type>(Context & ctx)

	COMMAND_TYPE_IMPL(BeginRenderPass)
	{
		ctx.SwitchRenderPass(m_RenderPass);
	}

	COMMAND_TYPE_IMPL(SetViewport)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		const SDL_GPUViewport vp{
			.x = m_Viewport.x0,
			.y = m_Viewport.y0,
			.w = m_Viewport.GetWidth(),
			.h = m_Viewport.GetHeight(),
			.min_depth = 0.0f,
			.max_depth = 1.0f,
		};
		SDL_SetGPUViewport(renderPass->GetHandle(), &vp);
	}
	COMMAND_TYPE_IMPL(BindGraphicsPipeline)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		SDL_BindGPUGraphicsPipeline(renderPass->GetHandle(), m_Pipeline);
	}

	COMMAND_TYPE_IMPL(BindMaterialInstance)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		m_MaterialInstance->PushFragmentConstants(ctx.GetMainCommandBuffer());
		m_MaterialInstance->Bind(renderPass->GetHandle());
	}

	COMMAND_TYPE_IMPL(BindVertexBuffers)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");

		auto* bindings = Alloca(SDL_GPUBufferBinding, m_Buffers.size());
		for (size_t i = 0; i < m_Buffers.size(); ++i)
		{
			bindings[i] = {
				.buffer = m_Buffers[i].GetHandle(),
			};
		}
		SDL_BindGPUVertexBuffers(
			renderPass->GetHandle(),
			0,
			bindings,
			NumCast<uint32>(m_Buffers.size()));
	}

	COMMAND_TYPE_IMPL(BindIndexBuffer)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		const SDL_GPUBufferBinding binding{
			.buffer = m_IBuffer->GetHandle(),
		};
		SDL_BindGPUIndexBuffer(renderPass->GetHandle(), &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
	}

	COMMAND_TYPE_IMPL(BindVertexStorageBuffers)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");

		auto* bindings = Alloca(SDL_GPUBuffer*, m_Buffers.size());
		for (size_t i = 0; i < m_Buffers.size(); ++i)
		{
			bindings[i] = m_Buffers[i].GetHandle();
		}

		SDL_BindGPUVertexStorageBuffers(
			renderPass->GetHandle(),
			0,
			bindings,
			NumCast<uint32>(m_Buffers.size()));
	}

	COMMAND_TYPE_IMPL(BindFragmentStorageBuffers)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");

		auto* bindings = Alloca(SDL_GPUBuffer*, m_Buffers.size());
		for (size_t i = 0; i < m_Buffers.size(); ++i)
		{
			bindings[i] = m_Buffers[i].GetHandle();
		}

		SDL_BindGPUFragmentStorageBuffers(
			renderPass->GetHandle(),
			0,
			bindings,
			NumCast<uint32>(m_Buffers.size()));
	}

	COMMAND_TYPE_IMPL(DrawPrimitives)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		SDL_DrawGPUPrimitives(
			renderPass->GetHandle(),
			m_DrawCall.m_NumVertices,
			m_DrawCall.m_NumInstances,
			m_DrawCall.m_FistVertex,
			m_DrawCall.m_FistInstance);
	}

	COMMAND_TYPE_IMPL(DrawIndexedPrimitives)
	{
		RenderPass* const renderPass = ctx.GetCurrentRenderPass();
		APOLLO_ASSERT(renderPass, "SetViewport command called, but no render pass in progress");
		SDL_DrawGPUIndexedPrimitives(
			renderPass->GetHandle(),
			m_IndexedDrawCall.m_NumIndices,
			m_IndexedDrawCall.m_NumInstances,
			m_IndexedDrawCall.m_FirstIndex,
			m_IndexedDrawCall.m_VertexOffset,
			m_IndexedDrawCall.m_FirstInstance);
	}

#undef COMMAND_TYPE_IMPL

	GPUCommand::GPUCommand(ECommandType t, const Buffer& buf)
		: m_Type(t)
	{
		APOLLO_ASSERT(buf, "Passed null buffer to GPU command");
		switch (t)
		{
		case ECommandType::BindIndexBuffer:
			APOLLO_ASSERT(
				buf.IsIndexBuffer(),
				"Command type is BindIndexBuffer but buffer is not an index buffer");
			m_IBuffer = &buf;
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

		m_Buffers = std::span{ &buf, 1 };
	}

	GPUCommand::GPUCommand(ECommandType type, std::span<const Buffer> buffers)
		: m_Buffers{ buffers }
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

	void GPUCommand::operator()(Context& ctx)
	{
		static constexpr std::array impl = []<size_t... Types>(std::index_sequence<Types...>)
		{
			using ArrayType = std::array<void (GPUCommand::*)(Context&), sizeof...(Types)>;
			return ArrayType{ &GPUCommand::Impl<static_cast<ECommandType>(Types)>... };
		}(std::make_index_sequence<size_t(ECommandType::NTypes)>());

		return (this->*impl[uint8(m_Type)])(ctx);
	}
} // namespace apollo::rdr