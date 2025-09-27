#pragma once

#include <PCH.hpp>
#include <core/TypeInfo.hpp>
#include <entt/entity/fwd.hpp>

namespace brk::ecs {
	template <class S>
	concept System = requires(S & instance, entt::registry& world)
	{
		{ instance.Update(world) };
	};

	class BRK_API SystemInstance
	{
		using UpdateFunc = void(void*, entt::registry&);
	public:
		template <System S, class... Args>
		static SystemInstance Create(Args&&... args) requires(std::is_constructible_v<S, Args...>)
		{
			S* ptr = new S{ std::forward<Args>(args)... };
			auto update = [](void* system, entt::registry& world)
			{
				static_cast<S*>(system)->Update(world);
			};
			auto deleteFunc = [](void* system)
			{
				delete static_cast<S*>(system);
			};
			SystemInstance res{ ptr, update, deleteFunc };
			return res;
		}

		~SystemInstance();
		SystemInstance(const SystemInstance&) = delete;
		SystemInstance(SystemInstance&& other) noexcept;

		SystemInstance& operator=(const SystemInstance&) = delete;
		SystemInstance& operator=(SystemInstance&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		void Swap(SystemInstance& other) noexcept;

		void Update(entt::registry& world);

		void Shutdown();

		template <class S>
		S* GetAs() noexcept
		{
			return reinterpret_cast<S*>(m_Ptr);
		}

		template <class S>
		const S* GetAs() const noexcept
		{
			return reinterpret_cast<S*>(m_Ptr);
		}

		[[nodiscard]] operator bool() const noexcept { return m_Ptr; }
		[[nodiscard]] bool operator==(const SystemInstance& other) const noexcept
		{
			return other.m_Ptr == m_Ptr;
		}

	private:
		SystemInstance(void* ptr, UpdateFunc* updateFunc, void (*deleteFunc)(void*));

		void* m_Ptr = nullptr;
		UpdateFunc* m_Update = nullptr;
		void (*m_Delete)(void*) = nullptr;
	};
} // namespace brk::ecs