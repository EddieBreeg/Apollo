#include "System.hpp"

namespace brk::ecs {
	SystemInstance::SystemInstance(
		void* ptr,
		void (*updateFunc)(void*, entt::registry&),
		void (*deleteFunc)(void*))
		: m_Ptr(ptr)
		, m_Update(updateFunc)
		, m_Delete(deleteFunc)
	{}

	void SystemInstance::Update(entt::registry& world)
	{
		if (m_Ptr) [[likely]]
			m_Update(m_Ptr, world);
	}

	SystemInstance::~SystemInstance()
	{
		Shutdown();
	}

	SystemInstance::SystemInstance(SystemInstance&& other) noexcept
		: m_Ptr(other.m_Ptr)
		, m_Update(other.m_Update)
		, m_Delete(other.m_Delete)
	{
		other.m_Ptr = nullptr;
		other.m_Update = nullptr;
		other.m_Delete = nullptr;
	}

	void SystemInstance::Swap(SystemInstance& other) noexcept
	{
		std::swap(m_Ptr, other.m_Ptr);
		std::swap(m_Update, other.m_Update);
		std::swap(m_Delete, other.m_Delete);
	}

	void SystemInstance::Shutdown()
	{
		if (m_Ptr)
		{
			m_Delete(m_Ptr);
			m_Ptr = nullptr;
			m_Update = nullptr;
			m_Delete = nullptr;
		}
	}
} // namespace brk::ecs