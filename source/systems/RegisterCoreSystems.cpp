#include "InputSystem.hpp"

#include <asset/AssetManager.hpp>
#include <ecs/Manager.hpp>
#include <systems/SceneLoadingSystem.hpp>

namespace apollo {
	void RegisterCoreSystems(App& app, ecs::Manager& manager, AssetManager& assetManager)
	{
		manager.AddSystem<inputs::System>(app);
		manager.AddSystem<SceneLoadingSystem>(assetManager);
	}
} // namespace apollo