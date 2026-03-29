#pragma once

/** \file Context.hpp */

#include <PCH.hpp>
#include <core/Singleton.hpp>

#include "Command.hpp"
#include "Device.hpp"
#include "Pixel.hpp"
#include <core/Queue.hpp>

struct SDL_Window;
struct SDL_GPUCommandBuffer;
struct SDL_GPUSampler;
struct SDL_GPUTexture;

namespace apollo {
	class Window;
}

/**
 * \namespace apollo::rdr
 * \brief Rendering APIs
 */
namespace apollo::rdr {
	class RenderPass;

	/**
	 * \brief Global rendering context. This is the central rendering API you'll be interacting with
	 * most of the time.
	 */
	class Context : public Singleton<Context>
	{
	public:
		APOLLO_API ~Context();

		GPUDevice& GetDevice() noexcept { return m_Device; }
		Window& GetWindow() noexcept { return m_Window; }

		/** \brief Creates the main command buffer, and acquires the swapchain texture. Called by
		 * the main App class
		 */
		APOLLO_API void BeginFrame();

		/** \anchor rdr-derrefered-api */
		/** \name Deferred commands
		 * \brief These functions add a command to the internal queue.
		 * \details For practical reasons, GPU commands are not recorded into the command buffer
		 * directly. They are first added to a queue and processed at the end of the frame.
		 * @{ */

		/// Starts a new render pass. If a render pass was already in progress, it is ended first.
		void BeginRenderPass(RenderPass& renderPass) { m_CommandQueue.AddEmplace(renderPass); }
		/// Sets the viewport region within the current render pass
		void SetViewport(const RectF& viewport) { m_CommandQueue.AddEmplace(viewport); }
		/// Sets the scissor region within the current render pass
		void SetScissor(const ScissorCommand& scissor) { m_CommandQueue.AddEmplace(scissor); }

		/**
		 * \brief Pushes vertex shader constants to the GPU.
		 * \details The data will be valid until either this command is called again, or the command
		 * buffer is submitted.
		 * \param args: The data to push, optionally followed by the constant block index to use (0
		 * by default)
		 */
		template <class... Args>
		void PushVertexShaderConstants(Args&&... args)
		{
			m_CommandQueue.AddEmplace(EShaderStage::Vertex, std::forward<Args>(args)...);
		}
		/**
		 * \brief Pushes fragment shader constants to the GPU.
		 * \details The data will be valid until either this command is called again, or the command
		 * buffer is submitted.
		 * \param args: The data to push, optionally followed by the constant block index to use (0
		 * by default)
		 */
		template <class... Args>
		void PushFragmentShaderConstants(Args&&... args)
		{
			m_CommandQueue.AddEmplace(EShaderStage::Fragment, std::forward<Args>(args)...);
		}

		/**
		 * \brief Binds a graphics pipeline to the current render pass
		 * \note If you're looking to bind a whole material, including resources like textures etc,
		 * use \ref apollo::rdr::Context::BindMaterialInstance "BindMaterialInstance()" instead.
		 */
		void BindGraphicsPipeline(SDL_GPUGraphicsPipeline* pipeline)
		{
			m_CommandQueue.AddEmplace(pipeline);
		}

		/**
		 * \brief Binds a whole material to the current render pass: the graphics pipeline along
		 * with textures/samplers and fragment shader constants
		 */
		void BindMaterialInstance(const MaterialInstance& mat) { m_CommandQueue.AddEmplace(mat); }

		/// Binds an index buffer to the current render pass
		void BindIndexBuffer(const Buffer& buffer)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindIndexBuffer, buffer);
		}
		/// Binds a vertex buffer to the current render pass
		void BindVertexBuffer(const Buffer& buffer)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindVertexBuffers, buffer);
		}
		/// Binds a vertex storage buffer to the current render pass
		void BindVertexStorageBuffer(const Buffer& buffer)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindVertexStorageBuffers, buffer);
		}
		/// Binds a fragment storage buffer to the current render pass
		void BindFragmentStorageBuffer(const Buffer& buffer)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindFragmentStorageBuffers, buffer);
		}
		/// Binds multiple vertex buffers at once
		void BindVertexBuffers(std::span<const Buffer> buffers)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindVertexBuffers, buffers);
		}
		/// Binds multiple vertex storage buffers at once
		void BindVertexStorageBuffers(std::span<const Buffer> buffers)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindVertexStorageBuffers, buffers);
		}
		/// Binds multiple fragment storage buffers at once
		void BindFragmentStorageBuffers(std::span<const Buffer> buffers)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindFragmentStorageBuffers, buffers);
		}
		/// Issues a direct draw call
		void DrawPrimitives(const DrawCall& call) { m_CommandQueue.AddEmplace(call); }
		/// Issues an indexed draw call
		void DrawIndexedPrimitives(const IndexedDrawCall& call) { m_CommandQueue.AddEmplace(call); }

		/// Used by the editor to draw the ImGui UI layer
		void DrawImGuiLayer(const ImGuiDrawCommand& call) { m_CommandQueue.AddEmplace(call); }

		/**
		 * \brief Submits a custom command, as a function object.
		 * \pre The function object must be a \ref apollo::meta::SmallTrivial "small trivial"
		 */
		template <class F>
		void AddCustomCommand(F&& cmd) requires(requires { cmd(*this); })
		{
			m_CommandQueue.AddEmplace(std::forward<F>(cmd));
		}
		/** @} */

		/// Returns `nullptr` if called outside of BeginFrame/EndFrame
		[[nodiscard]] SDL_GPUCommandBuffer* GetMainCommandBuffer() noexcept
		{
			return m_MainCommandBuffer;
		}
		/// Returns `nullptr` if called outside of BeginFrame/EndFrame
		[[nodiscard]] SDL_GPUTexture* GetSwapchainTexture() noexcept { return m_SwapchainTexture; }

		/// Processes the command queue and submits the command buffer
		APOLLO_API void EndFrame();

		[[nodiscard]] EPixelFormat GetSwapchainTextureFormat() const noexcept
		{
			return m_SwapchainFormat;
		}

		/**
		 * \brief This sampler uses linear filtering to sample a texture.
		 */
		[[nodiscard]] SDL_GPUSampler* GetDefaultSampler() noexcept { return m_DefaultSampler; }

		/**
		 * \brief Returns the current render pass, or null if pass is in progress
		 */
		[[nodiscard]] RenderPass* GetCurrentRenderPass() noexcept { return m_RenderPass; }

	private:
		/**
		 * Switches render pass. If a render pass was currently in progress, End is called on
		 * it.
		 * \param renderPass: The new render pass to switch to. If not nullptr, Begin is called
		 * on it.
		 */
		APOLLO_API void SwitchRenderPass(RenderPass* renderPass = nullptr);

		APOLLO_API Context(
			EBackend backend,
			Window& window,
			bool gpuDebug = false,
			bool vSync = false);

		friend class Singleton<Context>;
		friend class GPUCommand;

		static APOLLO_API std::unique_ptr<Context> s_Instance;

		GPUDevice m_Device;
		Window& m_Window;

		SDL_GPUCommandBuffer* m_MainCommandBuffer = nullptr;
		SDL_GPUTexture* m_SwapchainTexture = nullptr;
		EPixelFormat m_SwapchainFormat = EPixelFormat::Invalid;
		SDL_GPUSampler* m_DefaultSampler = nullptr;
		RenderPass* m_RenderPass = nullptr;
		Queue<GPUCommand> m_CommandQueue;
	};
} // namespace apollo::rdr