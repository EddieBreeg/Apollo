#pragma once

#include <PCH.hpp>

#include "System.hpp"

#include <core/Assert.hpp>
#include <core/NumConv.hpp>
#include <core/Singleton.hpp>
#include <core/TypeInfo.hpp>
#include <entt/entity/registry.hpp>
#include <vector>

/** \file Manager.hpp */

namespace apollo {
	class GameTime;
}

/**
 * \namespace apollo::ecs
 * \brief ECS (Entitity-Component-System) library
 */
namespace apollo::ecs {
	/**
	 * \brief Holds all ECS system instances and the entity world.
	 */
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

		/**
		 * \brief Adds a system to the manager. This system will automatically get updated every
		 * frame.
		 * \tparam S: The system type
		 * \param args: The arguments to forward to the constructor
		 */
		template <System S, class... T>
		S& AddSystem(T&&... args)
		{
			APOLLO_ASSERT(
				GetSystemIndex<S>() == NumCast<uint32>(m_Systems.size()),
				"Trying to add ECS system twice");
			return *m_Systems.emplace_back(SystemInstance::Create<S>(std::forward<T>(args)...))
						.template GetAs<S>();
		}

		/**
		 * \brief Calls the PostInit() method on all systems that define it.
		 * \note This is called by the App class at the end of the initialization phase.
		 * Calling this function is \b not your responsibility as a user.
		 */
		APOLLO_API void PostInit();
		/**
		 * \brief Updates all ECS systems, in the order they were added.
		 * \param t: The global game timer.
		 */
		APOLLO_API void Update(const GameTime& t);
		/**
		 * \brief Grants access to the entity world object. You don't usually need to call this
		 * function.
		 */
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