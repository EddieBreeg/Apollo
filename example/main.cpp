#include "CameraSystem.hpp"
#include "UiSystem.hpp"
#include "VisualSystem.hpp"
#include <editor/asset/Manager.hpp>
#include <ui/Context.hpp>
#include <ui/Renderer.hpp>

namespace apollo::demo {
	apollo::EAppResult Init(const apollo::EntryPoint&, apollo::App& app)
	{
		spdlog::set_level(spdlog::level::trace);

		auto* compRegistry = ecs::ComponentRegistry::GetInstance();
		compRegistry->RegisterComponent<MeshComponent>();
		compRegistry->RegisterComponent<GridComponent>();

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

		manager.AddSystem<UiSystem>(
			renderer,
			ui::Context::s_Instance,
			rdr::ui::Renderer::s_Instance,
			visualSystem.GetTargetViewport());

		return EAppResult::Continue;
	}
} // namespace apollo::demo

apollo::EntryPoint apollo::GetEntryPoint(std::span<const char*>)
{
	return apollo::EntryPoint{
		.m_AppName = "Apollo Example",
		.m_OnInit = apollo::demo::Init,
	};
}