#pragma once

#include <PCH.hpp>

#include "System.hpp"

#include <core/Assert.hpp>
#include <core/Singleton.hpp>
#include <core/TypeInfo.hpp>
#include <entt/entity/registry.hpp>
#include <vector>

namespace apollo {
	class GameTime;
}

namespace apollo::ecs {
	class Manager : public Singleton<Manager>
	{
		struct IndexGen
		{
			APOLLO_API static uint32 GetNext() noexcept;
		};
		template <System S>
		using SystemIndex = TypeIndex<S, IndexGen>;

	public:
		APOLLO_API ~Manager() = default;

		template <System S, class... T>
		S& AddSystem(T&&... args)
		{
			APOLLO_ASSERT(
				GetSystemIndex<S>() == (uint32)m_Systems.size(),
				"Trying to add ECS system twice");
			return *m_Systems.emplace_back(SystemInstance::Create<S>(std::forward<T>(args)...))
						.template GetAs<S>();
		}

		APOLLO_API void PostInit();
		APOLLO_API void Update(const GameTime&);
		[[nodiscard]] entt::registry& GetEntityWorld() noexcept { return m_World; }

	private:
		template <System S>
		static uint32 GetSystemIndex() noexcept
		{
			static const uint32 index = SystemIndex<S>::GetValue();
			return index;
		}

		friend class Singleton<Manager>;
		Manager() = default;
		static APOLLO_API std::unique_ptr<Manager> s_Instance;

		entt::registry m_World;
		std::vector<SystemInstance> m_Systems;
	};
} // namespace apollo::ecs