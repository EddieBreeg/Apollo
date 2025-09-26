#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <core/Window.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <imgui.h>
#include <rendering/Device.hpp>
#include <rendering/Renderer.hpp>
#include <rendering/Texture.hpp>

namespace ImGui {
	void ShowDemoWindow(bool* p_open);
}

namespace {
	struct TestSystem
	{
		brk::Window& m_Window;
		brk::rdr::Renderer& m_Renderer;
		brk::AssetRef<brk::rdr::Texture2D> m_Texture;

		void Update()
		{
			if (!m_Window) [[unlikely]]
				return;

			static bool firstFrame = true;
			if (firstFrame)
			{
				using namespace brk;
				using namespace brk::ulid_literal;

				firstFrame = false;
				m_Texture = AssetManager::GetInstance()->GetAsset<rdr::Texture2D>(
					"01K61BK2C2QZS0A18PWHGARASK"_ulid);
			}

			brk::rdr::GPUDevice& device = m_Renderer.GetDevice();

			auto* swapchainTexture = m_Renderer.GetSwapchainTexture();

			if (!swapchainTexture)
				return;

			auto* mainCommandBuffer = m_Renderer.GetMainCommandBuffer();

			const SDL_GPUColorTargetInfo targetInfo{
				.texture = swapchainTexture,
				.clear_color = { .1f, .1f, .1f, 1 },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.cycle = true,
			};

			SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
				mainCommandBuffer,
				&targetInfo,
				1,
				nullptr);

			SDL_EndGPURenderPass(renderPass);

			ImGui::ShowDemoWindow(nullptr);
		}
	};

	brk::EAppResult Init(const brk::EntryPoint&, brk::App& app)
	{
		spdlog::set_level(spdlog::level::trace);
		auto& renderer = *brk::rdr::Renderer::GetInstance();
		ImGui::SetCurrentContext(app.GetImGuiContext());
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