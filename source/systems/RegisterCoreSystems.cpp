#include "InputSystem.hpp"

#include <ecs/Manager.hpp>

namespace brk
{
	void RegisterCoreSystems(App& app, ecs::Manager& manager)
	{
		manager.AddSystem<inputs::System>(app);
	}
}