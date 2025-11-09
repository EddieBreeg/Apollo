#pragma once

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

namespace apollo::rdr {
	class RenderPass;

	class Context : public Singleton<Context>
	{
	public:
		APOLLO_API ~Context();

		GPUDevice& GetDevice() noexcept { return m_Device; }
		Window& GetWindow() noexcept { return m_Window; }

		// Creates the main command buffer, and acquires the swapchain texture
		APOLLO_API void BeginFrame();

		void BeginRenderPass(RenderPass& renderPass) { m_CommandQueue.AddEmplace(renderPass); }
		void SetViewport(const RectF& viewport) { m_CommandQueue.AddEmplace(viewport); }

		template <class... Args>
		void PushVertexShaderConstants(Args&&... args)
		{
			m_CommandQueue.AddEmplace(EShaderStage::Vertex, std::forward<Args>(args)...);
		}
		template <class... Args>
		void PushFragmentShaderConstants(Args&&... args)
		{
			m_CommandQueue.AddEmplace(EShaderStage::Fragment, std::forward<Args>(args)...);
		}

		void BindGraphicsPipeline(SDL_GPUGraphicsPipeline* pipeline)
		{
			m_CommandQueue.AddEmplace(pipeline);
		}

		void BindMaterialInstance(MaterialInstance& mat) { m_CommandQueue.AddEmplace(mat); }

		void BindIndexBuffer(const Buffer& buffer)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindIndexBuffer, buffer);
		}
		void BindVertexBuffer(const Buffer& buffer)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindVertexBuffers, buffer);
		}
		void BindVertexStorageBuffer(const Buffer& buffer)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindVertexStorageBuffers, buffer);
		}
		void BindFragmentStorageBuffer(const Buffer& buffer)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindFragmentStorageBuffers, buffer);
		}
		void BindVertexBuffers(std::span<const Buffer> buffers)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindVertexBuffers, buffers);
		}
		void BindVertexStorageBuffers(std::span<const Buffer> buffers)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindVertexStorageBuffers, buffers);
		}
		void BindFragmentStorageBuffers(std::span<const Buffer> buffers)
		{
			m_CommandQueue.AddEmplace(GPUCommand::BindFragmentStorageBuffers, buffers);
		}
		void DrawPrimitives(const DrawCall& call) { m_CommandQueue.AddEmplace(call); }
		void DrawIndexedPrimitives(const IndexedDrawCall& call) { m_CommandQueue.AddEmplace(call); }
		void DrawImGuiLayer(const ImGuiDrawCommand& call) { m_CommandQueue.AddEmplace(call); }

		// Returns null if called outside of BeginFrame/EndFrame
		[[nodiscard]] SDL_GPUCommandBuffer* GetMainCommandBuffer() noexcept
		{
			return m_MainCommandBuffer;
		}
		// Returns null if called outside of BeginFrame/EndFrame
		[[nodiscard]] SDL_GPUTexture* GetSwapchainTexture() noexcept { return m_SwapchainTexture; }

		// Submits the main command buffer
		APOLLO_API void EndFrame();

		[[nodiscard]] EPixelFormat GetSwapchainTextureFormat() const noexcept
		{
			return m_SwapchainFormat;
		}

		[[nodiscard]] SDL_GPUSampler* GetDefaultSampler() noexcept { return m_DefaultSampler; }

	private:
		/**
		 * Switches render pass. If a render pass was currently in progress, End is called on
		 * it.
		 * \param renderPass: The new render pass to switch to. If not nullptr, Begin is called
		 * on it.
		 */
		APOLLO_API void SwitchRenderPass(RenderPass* renderPass = nullptr);
		[[nodiscard]] RenderPass* GetCurrentRenderPass() noexcept { return m_RenderPass; }

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