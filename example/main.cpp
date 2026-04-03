#include "CameraSystem.hpp"
#include "UiSystem.hpp"
#include "VisualSystem.hpp"
#include <editor/asset/Manager.hpp>
#include <ui/Context.hpp>
#include <ui/Renderer.hpp>

namespace apollo::demo {
	struct Demo : public IGameState
	{
		EAppResult OnInit(const apollo::EntryPoint&, apollo::App& app) override
		{
			spdlog::set_level(spdlog::level::trace);

			// Component registration
			auto* compRegistry = ecs::ComponentRegistry::GetInstance();
			compRegistry->RegisterComponent<MeshComponent>();
			compRegistry->RegisterComponent<GridComponent>();

			// System initialization
			auto& renderer = *apollo::rdr::Context::GetInstance();
			auto& manager = *apollo::ecs::Manager::GetInstance();
			auto& camSystem = manager.AddSystem<apollo::demo::CameraSystem>(renderer.GetWindow());
			auto& visualSystem = manager.AddSystem<VisualSystem>(
				app.GetMainWindow(),
				renderer,
				camSystem);

			visualSystem.m_Inspector.m_AssetManager = static_cast<editor::AssetManager*>(
				IAssetManager::GetInstance());
			auto& world = manager.GetEntityWorld();
			world.emplace<SceneSwitchRequestComponent>(
				world.create(),
				"01KMAX1C7SPE9BNEW848XEZT4D"_ulid);

			m_UiContext = &ui::Context::s_Instance;
			m_UiRenderer = &rdr::ui::Renderer::s_Instance;

			manager.AddSystem<UiSystem>(
				renderer,
				*m_UiContext,
				*m_UiRenderer,
				App::GetShaderCompiler(),
				visualSystem.GetTargetViewport());

			return EAppResult::Continue;
		}

		void OnQuit(App&) override
		{
			APOLLO_LOG_INFO("Exiting Demo app");
			m_UiRenderer->Reset();
			m_UiContext->Reset();
		}

		rdr::ui::Renderer* m_UiRenderer = nullptr;
		ui::Context* m_UiContext = nullptr;
	};
} // namespace apollo::demo

apollo::EntryPoint apollo::GetEntryPoint(std::span<const char*>)
{
	return apollo::EntryPoint{
		.m_AppName = "Apollo Example",
		.m_GameState = std::make_unique<apollo::demo::Demo>(),
	};
}