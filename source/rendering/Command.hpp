#pragma once

/** \file Command.hpp
 * \brief Deferred GPU commands
 */

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

	/// Direct draw command
	struct DrawCall
	{
		uint32 m_NumVertices = 0;
		uint32 m_NumInstances = 1;
		uint32 m_FistVertex = 0;
		uint32 m_FistInstance = 0;
	};

	/// Indexed draw command
	struct IndexedDrawCall
	{
		uint32 m_NumIndices = 0;
		uint32 m_NumInstances = 1;
		uint32 m_FirstIndex = 0;
		int32 m_VertexOffset = 0;
		uint32 m_FirstInstance = 0;
	};

	/// Used to draw the ImGui layer
	struct ImGuiDrawCommand
	{
		ImDrawData* m_DrawData = nullptr;
		float4 m_ClearColor = {};
		bool m_ClearTarget = true;
	};

	/// Scissor command
	struct ScissorCommand
	{
		int32 x = 0;
		int32 y = 0;
		int32 w = 0;
		int32 h = 0;
	};

	/**
	 * \brief Deferred GPU command interface
	 * \details This is the interface used internally by the \ref Context "rendering context" to
	 * submit commands to the GPU. Commands are stored in a queue, and get recorded into the GPU
	 * command buffer at the end of the frame.
	 * \note You don't typically need to interact with this API, since the Context does this on your
	 * behalf
	 * \sa Context
	 */
	class GPUCommand
	{
	public:
		/// Identifies
		enum ECommandType : int8
		{
			Invalid = -1,
			// Shader constants
			PushVertexShaderConstants,
			PushFragmentShaderConstants,

			// Pass commands
			BeginRenderPass,
			SetViewport,
			SetScissor,

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

		/** \name  PushVertexShaderConstants/PushFragmentShaderConstants commands
		 * @{ */
		APOLLO_API GPUCommand(EShaderStage stage, const void* data, size_t size, uint32 slot = 0);
		template <class T>
		GPUCommand(EShaderStage stage, std::span<const T> data, uint32 slot = 0)
			: GPUCommand(stage, data.data(), data.size_bytes(), slot)
		{}
		template <class T>
		GPUCommand(EShaderStage stage, const T& data, uint32 slot = 0)
			: GPUCommand(stage, &data, sizeof(data), slot)
		{}
		/** @} */

		/// BeginRenderPass command
		explicit GPUCommand(RenderPass& renderPass) noexcept
			: m_Storage{ .m_RenderPass = &renderPass }
			, m_Type(ECommandType::BeginRenderPass)
		{}

		/// SetViewport command
		explicit GPUCommand(const RectF& viewport) noexcept
			: m_Storage{ .m_Viewport{ viewport } }
			, m_Type(ECommandType::SetViewport)
		{}

		/// SetScissor command
		explicit GPUCommand(const ScissorCommand& scissor) noexcept
			: m_Storage{ .m_Scissor{ scissor } }
			, m_Type(ECommandType::SetScissor)
		{}

		/// BindGraphicsPipeline command
		APOLLO_API explicit GPUCommand(SDL_GPUGraphicsPipeline* pipeline) noexcept
			: m_Storage{ .m_Pipeline = pipeline }
			, m_Type(ECommandType::BindGraphicsPipeline)
		{}

		/// BindMaterialInstance command
		APOLLO_API explicit GPUCommand(const MaterialInstance& mat) noexcept
			: m_Storage{ .m_MaterialInstance = &mat }
			, m_Type(ECommandType::BindMaterialInstance)
		{}

		/** \name Buffer bind commands
		 * @{ */
		APOLLO_API GPUCommand(ECommandType type, const Buffer& buffer);
		APOLLO_API GPUCommand(ECommandType type, std::span<const Buffer> buffers);
		/** @} */

		/** \name Draw commands
		 * @{ */

		/// DrawPrimitives command
		explicit GPUCommand(const DrawCall& drawCall) noexcept
			: m_Storage{ .m_DrawCall{ drawCall } }
			, m_Type(ECommandType::DrawPrimitives)
		{}

		/// DrawIndexedPrimitives command
		explicit GPUCommand(const IndexedDrawCall& drawCall) noexcept
			: m_Storage{ .m_IndexedDrawCall{ drawCall } }
			, m_Type(ECommandType::DrawIndexedPrimitives)
		{}

		/// DrawImGuiLayer command
		explicit GPUCommand(const ImGuiDrawCommand& cmd) noexcept
			: m_Storage{ .m_ImGuiDrawCall{ cmd } }
			, m_Type(DrawImGuiLayer)
		{}

		/** @} */

		/**
		 * \brief Custom command in the form of a callback.
		 * \tparam F: The function object type. Must be a meta::SmallTrivial type, invokable with
		 * single argument of type Context&
		 */
		template <meta::SmallTrivial F>
		explicit GPUCommand(F&& cmd) noexcept requires(std::is_invocable_r_v<void, F, Context&>)
			: m_Storage{ .m_Custom{ std::forward<F>(cmd) } }
			, m_Type(ECommandType::Custom)
		{}

		APOLLO_API GPUCommand(GPUCommand&&) noexcept;
		APOLLO_API GPUCommand& operator=(GPUCommand&& other) noexcept;
		APOLLO_API ~GPUCommand();

		/**
		 * \brief Invokes the command
		 */
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
			ScissorCommand m_Scissor;
			DrawCall m_DrawCall;
			IndexedDrawCall m_IndexedDrawCall;
			ImGuiDrawCommand m_ImGuiDrawCall;
			CustomCommand m_Custom;
			char _unused;
		} m_Storage;
		ECommandType m_Type;
	};
} // namespace apollo::rdr