#include <systems/TransformComponent.hpp>

namespace apollo::editor {
	void RegisterComponents(ecs::ComponentRegistry& registry)
	{
		registry.RegisterComponent<TransformComponent>();
	}
} // namespace apollo::editor