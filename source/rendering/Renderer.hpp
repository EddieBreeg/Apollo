#pragma once

#include <PCH.hpp>
#include <core/Singleton.hpp>

#include "Device.hpp"

struct SDL_Window;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;

namespace brk {
	class Window;
}

namespace brk::rdr {
	class Renderer : public Singleton<Renderer>
	{
	public:
		~Renderer() = default;

		float4 m_ClearColor{};

		GPUDevice& GetDevice() noexcept { return m_Device; }

		// Creates the main command buffer, and acquires the swapchain texture
		BRK_API void BeginFrame();

		// Returns null if called outside of BeginFrame/EndFrame
		[[nodiscard]] SDL_GPUCommandBuffer* GetMainCommandBuffer() noexcept
		{
			return m_MainCommandBuffer;
		}
		// Returns null if called outside of BeginFrame/EndFrame
		[[nodiscard]] SDL_GPUTexture* GetSwapchainTexture() noexcept { return m_SwapchainTexture; }

		// Renders out ImGui content
		BRK_API void ImGuiRenderPass();

		// Submits the main command buffer
		BRK_API void EndFrame();

	private:
		Renderer(EBackend backend, Window& window, bool gpuDebug = false, bool vSync = false);
		friend class Singleton<Renderer>;

		static BRK_API std::unique_ptr<Renderer> s_Instance;

		GPUDevice m_Device;
		Window& m_Window;

		SDL_GPUCommandBuffer* m_MainCommandBuffer = nullptr;
		SDL_GPUTexture* m_SwapchainTexture = nullptr;
	};
} // namespace brk::rdr