#pragma once

#include <PCH.hpp>
#include <core/TypeInfo.hpp>
#include <entt/entity/fwd.hpp>

namespace apollo {
	class GameTime;
}

namespace apollo::ecs {
	template <class S>
	concept System = requires(S & instance, entt::registry& world, const GameTime& time)
	{
		{ instance.Update(world, time) };
	};

	namespace _internal {
		template <class T>
		concept HasPostInit = requires(T & obj)
		{
			{ obj.PostInit() };
		};
	} // namespace _internal

	class APOLLO_API SystemInstance
	{
		using UpdateFunc = void(void*, entt::registry&, const GameTime&);

	public:
		template <System S, class... Args>
		static SystemInstance Create(Args&&... args) requires(std::is_constructible_v<S, Args...>)
		{
			S* ptr = new S{ std::forward<Args>(args)... };
			auto update = [](void* system, entt::registry& world, const GameTime& time)
			{
				static_cast<S*>(system)->Update(world, time);
			};
			auto deleteFunc = [](void* system)
			{
				delete static_cast<S*>(system);
			};
			void (*postInit)(void*) = nullptr;
			if constexpr (_internal::HasPostInit<S>)
			{
				postInit = [](void* ptr)
				{
					static_cast<S*>(ptr)->PostInit();
				};
			}
			SystemInstance res{
				VTable{
					.m_Update = update,
					.m_Delete = deleteFunc,
					.m_PostInit = postInit,
				},
				ptr,
			};
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

		void Update(entt::registry& world, const GameTime& time);

		void PostInit();
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
		struct VTable
		{
			UpdateFunc* m_Update = nullptr;
			void (*m_Delete)(void*) = nullptr;
			void (*m_PostInit)(void*) = nullptr;
		};

		SystemInstance(const VTable& impl, void* ptr);

		void* m_Ptr = nullptr;
		VTable m_Impl;
	};
} // namespace apollo::ecs