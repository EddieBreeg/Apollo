#pragma once

#include <PCH.hpp>

#include <core/Assert.hpp>
#include <span>

struct SDL_GPUGraphicsPipeline;

namespace apollo::rdr {
	class Buffer;
	class Context;
	class MaterialInstance;
	class RenderPass;

	struct DrawCall
	{
		uint32 m_NumVertices = 0;
		uint32 m_NumInstances = 1;
		uint32 m_FistVertex = 0;
		uint32 m_FistInstance = 0;
	};

	struct IndexedDrawCall
	{
		uint32 m_NumIndices = 0;
		uint32 m_NumInstances = 1;
		uint32 m_FirstIndex = 0;
		int32 m_VertexOffset = 0;
		uint32 m_FirstInstance = 0;
	};

	class GPUCommand
	{
	public:
		enum ECommandType : uint8
		{
			// Pass commands
			BeginRenderPass,
			SetViewport,

			// Bind commands
			BindGraphicsPipeline,
			BindMaterialInstance, // Shortcut for binding a whole material in "one" command:
								  // graphics pipeline, textures etc
			BindIndexBuffer,
			BindVertexBuffers,
			BindVertexStorageBuffers,
			BindFragmentStorageBuffers,

			// Draw commands
			DrawPrimitives,
			DrawIndexedPrimitives,

			NTypes
		};

		explicit GPUCommand(RenderPass& renderPass)
			: m_RenderPass{ &renderPass }
			, m_Type(ECommandType::BeginRenderPass)
		{}

		explicit GPUCommand(const RectF& viewport)
			: m_Viewport{ viewport }
			, m_Type(ECommandType::SetViewport)
		{}

		APOLLO_API explicit GPUCommand(SDL_GPUGraphicsPipeline* pipeline)
			: m_Pipeline(pipeline)
			, m_Type(ECommandType::BindGraphicsPipeline)
		{}
		APOLLO_API explicit GPUCommand(MaterialInstance& mat)
			: m_MaterialInstance(&mat)
			, m_Type(ECommandType::BindMaterialInstance)
		{}

		APOLLO_API explicit GPUCommand(ECommandType type, const Buffer& buffer);
		APOLLO_API explicit GPUCommand(ECommandType type, std::span<const Buffer> buffers);

		explicit GPUCommand(const DrawCall& drawCall)
			: m_DrawCall{ drawCall }
			, m_Type(ECommandType::DrawPrimitives)
		{}

		explicit GPUCommand(const IndexedDrawCall& drawCall)
			: m_IndexedDrawCall{ drawCall }
			, m_Type(ECommandType::DrawIndexedPrimitives)
		{}

		GPUCommand(const GPUCommand&) = default;
		GPUCommand& operator=(const GPUCommand&) = default;
		~GPUCommand() = default;

		APOLLO_API void operator()(Context& ctx);

	private:
		template <ECommandType Type>
		void Impl(Context& ctx);

		union {
			RenderPass* m_RenderPass;
			SDL_GPUGraphicsPipeline* m_Pipeline;
			MaterialInstance* m_MaterialInstance;
			std::span<const Buffer> m_Buffers;
			const Buffer* m_IBuffer;
			RectF m_Viewport;
			DrawCall m_DrawCall;
			IndexedDrawCall m_IndexedDrawCall;
		};
		ECommandType m_Type;
	};
} // namespace apollo::rdr