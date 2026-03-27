#include <ecs/Manager.hpp>

void InitMySystem(apollo::ecs::Manager& manager)
{
	manager.AddSystem<MySystem>(/* constructor argments...*/);
}