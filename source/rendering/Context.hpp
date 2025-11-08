#pragma once

#include <PCH.hpp>
#include <core/Singleton.hpp>

#include "Device.hpp"
#include "Pixel.hpp"

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

		float4 m_ClearColor{};
		bool m_ClearBackBuffer = false;

		GPUDevice& GetDevice() noexcept { return m_Device; }
		Window& GetWindow() noexcept { return m_Window; }

		// Creates the main command buffer, and acquires the swapchain texture
		APOLLO_API void BeginFrame();

		/**
		 * Switches render pass. If a render pass was currently in progress, End is called on it.
		 * \param renderPass: The new render pass to switch to. If not nullptr, Begin is called on
		 * it.
		 */
		APOLLO_API void SwitchRenderPass(RenderPass* renderPass = nullptr);
		[[nodiscard]] RenderPass* GetCurrentRenderPass() noexcept { return m_RenderPass; }

		// Returns null if called outside of BeginFrame/EndFrame
		[[nodiscard]] SDL_GPUCommandBuffer* GetMainCommandBuffer() noexcept
		{
			return m_MainCommandBuffer;
		}
		// Returns null if called outside of BeginFrame/EndFrame
		[[nodiscard]] SDL_GPUTexture* GetSwapchainTexture() noexcept { return m_SwapchainTexture; }

		// Renders out ImGui content
		APOLLO_API void ImGuiRenderPass();

		// Submits the main command buffer
		APOLLO_API void EndFrame();

		[[nodiscard]] EPixelFormat GetSwapchainTextureFormat() const noexcept
		{
			return m_SwapchainFormat;
		}

		[[nodiscard]] SDL_GPUSampler* GetDefaultSampler() noexcept { return m_DefaultSampler; }

	private:
		APOLLO_API Context(
			EBackend backend,
			Window& window,
			bool gpuDebug = false,
			bool vSync = false);
		friend class Singleton<Context>;

		static APOLLO_API std::unique_ptr<Context> s_Instance;

		GPUDevice m_Device;
		Window& m_Window;

		SDL_GPUCommandBuffer* m_MainCommandBuffer = nullptr;
		SDL_GPUTexture* m_SwapchainTexture = nullptr;
		EPixelFormat m_SwapchainFormat = EPixelFormat::Invalid;
		SDL_GPUSampler* m_DefaultSampler = nullptr;
		RenderPass* m_RenderPass = nullptr;
	};
} // namespace apollo::rdr