#pragma once

#include <PCH.hpp>

#include "ShaderInfo.hpp"
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

	struct ShaderConstantCommand
	{
		ShaderConstantStorage m_Data;
		uint32 m_Size;
		uint32 m_Slot = 0;
	};

	class GPUCommand
	{
	public:
		enum ECommandType : uint8
		{
			// Shader constants
			PushVertexShaderConstants,
			PushFragmentShaderConstants,

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

		APOLLO_API GPUCommand(EShaderStage stage, const void* data, size_t size, uint32 slot = 0);
		template <class T>
		GPUCommand(EShaderStage stage, std::span<const T> data, uint32 slot = 0)
			: GPUCommand(stage, data.data(), data.size_bytes(), slot)
		{}
		template <class T>
		GPUCommand(EShaderStage stage, const T& data, uint32 slot = 0)
			: GPUCommand(stage, &data, sizeof(data), slot)
		{}

		explicit GPUCommand(RenderPass& renderPass) noexcept
			: m_RenderPass{ &renderPass }
			, m_Type(ECommandType::BeginRenderPass)
		{}

		explicit GPUCommand(const RectF& viewport) noexcept
			: m_Viewport{ viewport }
			, m_Type(ECommandType::SetViewport)
		{}

		APOLLO_API explicit GPUCommand(SDL_GPUGraphicsPipeline* pipeline) noexcept
			: m_Pipeline(pipeline)
			, m_Type(ECommandType::BindGraphicsPipeline)
		{}
		APOLLO_API explicit GPUCommand(MaterialInstance& mat) noexcept
			: m_MaterialInstance(&mat)
			, m_Type(ECommandType::BindMaterialInstance)
		{}

		APOLLO_API GPUCommand(ECommandType type, const Buffer& buffer);
		APOLLO_API GPUCommand(ECommandType type, std::span<const Buffer> buffers);

		explicit GPUCommand(const DrawCall& drawCall) noexcept
			: m_DrawCall{ drawCall }
			, m_Type(ECommandType::DrawPrimitives)
		{}

		explicit GPUCommand(const IndexedDrawCall& drawCall) noexcept
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
			ShaderConstantCommand m_Constants;
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