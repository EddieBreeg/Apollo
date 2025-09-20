#include <SDL3/SDL_gpu.h>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <core/Window.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <rendering/Device.hpp>
#include <rendering/Renderer.hpp>

namespace {
	struct CommandBufferSubmitter
	{
		void operator()(SDL_GPUCommandBuffer* buf) { SDL_SubmitGPUCommandBuffer(buf); }
	};
	using CommandBufferRef = std::unique_ptr<SDL_GPUCommandBuffer, CommandBufferSubmitter>;
	struct TestSystem
	{
		brk::Window& m_Window;
		brk::rdr::Renderer& m_Renderer;

		void Update()
		{
			if (!m_Window) [[unlikely]]
				return;

			brk::rdr::GPUDevice& device = m_Renderer.GetDevice();

			CommandBufferRef mainCommandBuffer{ SDL_AcquireGPUCommandBuffer(device.GetHandle()) };
			DEBUG_CHECK(mainCommandBuffer)
			{
				BRK_LOG_ERROR("Failed to acquire command buffer: {}", SDL_GetError());
				return;
			}

			SDL_GPUTexture* swapchainTexture = nullptr;
			uint32 width = 0, height = 0;
			DEBUG_CHECK(SDL_WaitAndAcquireGPUSwapchainTexture(
				mainCommandBuffer.get(),
				m_Window.GetHandle(),
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
	};

	brk::EAppResult Init(const brk::EntryPoint&, brk::App& app)
	{
		auto& renderer = *brk::rdr::Renderer::GetInstance();
		auto& manager = *brk::ecs::Manager::GetInstance();
		manager.AddSystem<TestSystem>(app.GetMainWindow(), renderer);
		return brk::EAppResult::Continue;
	}
} // namespace

void brk::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Breakout Example";
	out_entry.m_OnInit = Init;
}