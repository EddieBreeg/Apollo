#include <asset/AssetManager.hpp>
#include <systems/SceneComponents.hpp>
#include <systems/TransformComponent.hpp>

namespace apollo::editor {
	void RegisterComponents(ecs::ComponentRegistry& registry)
	{
		registry.RegisterComponent<TransformComponent>();
		registry.RegisterComponent<SceneComponent>();
	}
} // namespace apollo::editor