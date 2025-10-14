#include "Manager.hpp"

namespace {
	uint32 g_SystemIndex = 0;
}

namespace apollo::ecs {
	std::unique_ptr<Manager> Manager::s_Instance;

	uint32 Manager::IndexGen::GetNext() noexcept
	{
		return g_SystemIndex++;
	}

	void Manager::PostInit()
	{
		for (SystemInstance& s : m_Systems)
			s.PostInit();
	}

	void Manager::Update(const GameTime& time)
	{
		for (SystemInstance& s : m_Systems)
		{
			s.Update(m_World, time);
		}
	}
} // namespace apollo::ecs