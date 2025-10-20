#include "Context.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <core/Window.hpp>

#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

namespace apollo::rdr {
	std::unique_ptr<Context> Context::s_Instance;

	Context::Context(EBackend backend, Window& window, bool gpuDebug, bool vSync)
		: m_Device(backend, gpuDebug)
		, m_Window(window)
	{
		if (!m_Device) [[unlikely]]
			return;

		// it is valid to initialized the Context without a window, we can use offline rendering
		if (!window)
			return;

		DEBUG_CHECK(m_Device.ClaimWindow(window))
		{
			APOLLO_LOG_CRITICAL("Failed to claim window for GPU device: {}", SDL_GetError());
		}

		SDL_SetGPUSwapchainParameters(
			m_Device.GetHandle(),
			m_Window.GetHandle(),
			SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
			vSync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE);
	}

	void Context::BeginFrame()
	{
		m_MainCommandBuffer = SDL_AcquireGPUCommandBuffer(m_Device.GetHandle());
		DEBUG_CHECK(m_MainCommandBuffer)
		{
			APOLLO_LOG_ERROR("Failed to acquire command buffer: {}", SDL_GetError());
			return;
		}
		if (!m_Window) [[unlikely]]
			return;

		DEBUG_CHECK(SDL_WaitAndAcquireGPUSwapchainTexture(
			m_MainCommandBuffer,
			m_Window.GetHandle(),
			&m_SwapchainTexture,
			nullptr,
			nullptr))
		{
			APOLLO_LOG_ERROR("Failed to acquire swapchain texture: {}", SDL_GetError());
			return;
		}
	}

	void Context::ImGuiRenderPass()
	{
		DEBUG_CHECK(m_MainCommandBuffer)
		{
			return;
		}
		if (!m_SwapchainTexture) [[unlikely]]
			return;

		ImGui::Render();
		auto* drawData = ImGui::GetDrawData();

		ImGui_ImplSDLGPU3_PrepareDrawData(drawData, m_MainCommandBuffer);
		const SDL_GPUColorTargetInfo targetInfo{
			.texture = m_SwapchainTexture,
			.clear_color =
				SDL_FColor{
					m_ClearColor.r,
					m_ClearColor.g,
					m_ClearColor.b,
					m_ClearColor.a,
				},
			.load_op = SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_STORE,
		};

		SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(
			m_MainCommandBuffer,
			&targetInfo,
			1,
			nullptr);
		ImGui_ImplSDLGPU3_RenderDrawData(drawData, m_MainCommandBuffer, pass);
		SDL_EndGPURenderPass(pass);
	}

	void Context::EndFrame()
	{
		m_SwapchainTexture = nullptr;

		if (m_MainCommandBuffer) [[likely]]
		{
			SDL_SubmitGPUCommandBuffer(m_MainCommandBuffer);
			m_MainCommandBuffer = nullptr;
		}
	}
} // namespace apollo::rdr