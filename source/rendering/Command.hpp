#pragma once

#include <PCH.hpp>

#include "ShaderInfo.hpp"
#include <core/Poly.hpp>
#include <core/TypeTraits.hpp>
#include <span>

struct ImDrawData;
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

	struct ImGuiDrawCommand
	{
		ImDrawData* m_DrawData = nullptr;
		float4 m_ClearColor = {};
		bool m_ClearTarget = true;
	};

	class GPUCommand
	{
	public:
		enum ECommandType : int8
		{
			Invalid = -1,
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
			DrawImGuiLayer,

			// Custom callback command
			Custom,

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
			: m_Storage{ .m_RenderPass = &renderPass }
			, m_Type(ECommandType::BeginRenderPass)
		{}

		explicit GPUCommand(const RectF& viewport) noexcept
			: m_Storage{ .m_Viewport{ viewport } }
			, m_Type(ECommandType::SetViewport)
		{}

		APOLLO_API explicit GPUCommand(SDL_GPUGraphicsPipeline* pipeline) noexcept
			: m_Storage{ .m_Pipeline = pipeline }
			, m_Type(ECommandType::BindGraphicsPipeline)
		{}
		APOLLO_API explicit GPUCommand(const MaterialInstance& mat) noexcept
			: m_Storage{ .m_MaterialInstance = &mat }
			, m_Type(ECommandType::BindMaterialInstance)
		{}

		APOLLO_API GPUCommand(ECommandType type, const Buffer& buffer);
		APOLLO_API GPUCommand(ECommandType type, std::span<const Buffer> buffers);

		explicit GPUCommand(const DrawCall& drawCall) noexcept
			: m_Storage{ .m_DrawCall{ drawCall } }
			, m_Type(ECommandType::DrawPrimitives)
		{}

		explicit GPUCommand(const IndexedDrawCall& drawCall) noexcept
			: m_Storage{ .m_IndexedDrawCall{ drawCall } }
			, m_Type(ECommandType::DrawIndexedPrimitives)
		{}

		explicit GPUCommand(const ImGuiDrawCommand& cmd) noexcept
			: m_Storage{ .m_ImGuiDrawCall{ cmd } }
			, m_Type(DrawImGuiLayer)
		{}

		template <meta::SmallTrivial F>
		explicit GPUCommand(F&& cmd) noexcept requires(std::is_invocable_r_v<void, F, Context&>)
			: m_Storage{ .m_Custom{ std::forward<F>(cmd) } }
			, m_Type(ECommandType::Custom)
		{}

		APOLLO_API GPUCommand(GPUCommand&&) noexcept;
		APOLLO_API GPUCommand& operator=(GPUCommand&& other) noexcept;
		APOLLO_API ~GPUCommand();

		APOLLO_API void operator()(Context& ctx);

	private:
		template <ECommandType Type>
		void Impl(Context& ctx);

		struct CustomCommand
		{
			template <class F>
			CustomCommand(F&& func)
				: m_Invoke(&poly::Invoke<std::decay_t<F>>)
			{
				new (m_Buf) std::decay_t<F>{ std::forward<F>(func) };
			}
			alignas(std::max_align_t) char m_Buf[2 * sizeof(void*)];
			void (*m_Invoke)(void*, Context&);
		};

		struct ShaderConstantCommand
		{
			uint32 m_Size;
			uint32 m_Slot = 0;
			alignas(std::max_align_t) uint8 m_Buffer[1];
		};

		union Storage {
			ShaderConstantCommand* m_Constants;
			RenderPass* m_RenderPass;
			SDL_GPUGraphicsPipeline* m_Pipeline;
			const MaterialInstance* m_MaterialInstance;
			std::span<const Buffer> m_Buffers;
			const Buffer* m_IBuffer;
			RectF m_Viewport;
			DrawCall m_DrawCall;
			IndexedDrawCall m_IndexedDrawCall;
			ImGuiDrawCommand m_ImGuiDrawCall;
			CustomCommand m_Custom;
			char _unused;
		} m_Storage;
		ECommandType m_Type;
	};
} // namespace apollo::rdr