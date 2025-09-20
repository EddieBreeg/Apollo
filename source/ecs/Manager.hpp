#pragma once

#include <PCH.hpp>

#include "System.hpp"

#include <core/Assert.hpp>
#include <core/Singleton.hpp>
#include <core/TypeInfo.hpp>
#include <vector>

namespace brk::ecs {
	class BRK_API Manager : public Singleton<Manager>
	{
		struct IndexGen
		{
			BRK_API static uint32 GetNext() noexcept;
		};
		template <System S>
		using SystemIndex = TypeIndex<S, IndexGen>;

	public:
		~Manager() = default;

		template <System S, class... T>
		S& AddSystem(T&&... args)
		{
			BRK_ASSERT(
				GetSystemIndex<S>() == (uint32)m_Systems.size(),
				"Trying to add ECS system twice");
			return *m_Systems.emplace_back(SystemInstance::Create<S>(std::forward<T>(args)...))
						.template GetAs<S>();
		}

		void Update();

	private:
		template <System S>
		static uint32 GetSystemIndex() noexcept
		{
			static const uint32 index = SystemIndex<S>::GetValue();
			return index;
		}

		friend class Singleton<Manager>;
		Manager() = default;
		static std::unique_ptr<Manager> s_Instance;

		std::vector<SystemInstance> m_Systems;
	};
} // namespace brk::ecs