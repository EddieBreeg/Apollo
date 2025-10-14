#include "System.hpp"

namespace apollo::ecs {
	SystemInstance::SystemInstance(const VTable& impl, void* ptr)
		: m_Ptr(ptr)
		, m_Impl(impl)
	{}

	void SystemInstance::Update(entt::registry& world, const GameTime& time)
	{
		if (m_Ptr) [[likely]]
			m_Impl.m_Update(m_Ptr, world, time);
	}

	SystemInstance::~SystemInstance()
	{
		Shutdown();
	}

	SystemInstance::SystemInstance(SystemInstance&& other) noexcept
		: m_Ptr(other.m_Ptr)
		, m_Impl(other.m_Impl)
	{
		other.m_Ptr = nullptr;
		other.m_Impl = {};
	}

	void SystemInstance::Swap(SystemInstance& other) noexcept
	{
		std::swap(m_Ptr, other.m_Ptr);
		std::swap(m_Impl, other.m_Impl);
	}

	void SystemInstance::Shutdown()
	{
		if (m_Ptr)
		{
			m_Impl.m_Delete(m_Ptr);
			m_Ptr = nullptr;
			m_Impl = {};
		}
	}

	void SystemInstance::PostInit()
	{
		if (m_Ptr && m_Impl.m_PostInit)
			m_Impl.m_PostInit(m_Ptr);
	}
} // namespace apollo::ecs