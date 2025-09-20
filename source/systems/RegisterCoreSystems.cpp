#include "InputSystem.hpp"

#include <ecs/Manager.hpp>

namespace brk
{
	void RegisterCoreSystems(ecs::Manager& manager)
	{
		manager.AddSystem<inputs::System>();
	}
}