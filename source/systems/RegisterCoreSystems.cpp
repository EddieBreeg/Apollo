#include "InputSystem.hpp"

#include <ecs/Manager.hpp>

namespace apollo {
	void RegisterCoreSystems(App& app, ecs::Manager& manager)
	{
		manager.AddSystem<inputs::System>(app);
	}
} // namespace apollo