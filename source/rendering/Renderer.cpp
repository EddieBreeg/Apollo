#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <core/Window.hpp>

namespace {
	struct CommandBufferSubmitter
	{
		void operator()(SDL_GPUCommandBuffer* buf) { SDL_SubmitGPUCommandBuffer(buf); }
	};
	using CommandBufferRef = std::unique_ptr<SDL_GPUCommandBuffer, CommandBufferSubmitter>;
} // namespace

namespace brk::rdr {
	std::unique_ptr<Renderer> Renderer::s_Instance;

	Renderer::Renderer(EBackend backend, Window& window, bool gpuDebug)
		: m_Device(backend, gpuDebug)
		, m_WinHandle(window.GetHandle())
	{
		if (!m_Device) [[unlikely]]
			return;

		// it is valid to initialized the renderer without a window, we can use offline rendering
		if (!window)
			return;

		DEBUG_CHECK(m_Device.ClaimWindow(window))
		{
			BRK_LOG_CRITICAL("Failed to claim window for GPU device: {}", SDL_GetError());
		}
	}

	void Renderer::Update()
	{
		if (!m_WinHandle) [[unlikely]]
			return;

		CommandBufferRef mainCommandBuffer{ SDL_AcquireGPUCommandBuffer(m_Device.GetHandle()) };
		DEBUG_CHECK(mainCommandBuffer)
		{
			BRK_LOG_ERROR("Failed to acquire command buffer: {}", SDL_GetError());
			return;
		}

		SDL_GPUTexture* swapchainTexture = nullptr;
		uint32 width = 0, height = 0;
		DEBUG_CHECK(SDL_WaitAndAcquireGPUSwapchainTexture(
			mainCommandBuffer.get(),
			m_WinHandle,
			&swapchainTexture,
			&width,
			&height))
		{
			BRK_LOG_ERROR("Failed to acquire swapchain texture: {}", SDL_GetError());
			return;
		}

		if (!swapchainTexture)
			return;

		const SDL_GPUColorTargetInfo targetInfo{
			.texture = swapchainTexture,
			.clear_color = { 0, 0, 0, 1 },
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.cycle = true,
		};

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			mainCommandBuffer.get(),
			&targetInfo,
			1,
			nullptr);

		SDL_EndGPURenderPass(renderPass);
	}
} // namespace brk::rdr