#pragma once

#include <PCH.hpp>
#include <chrono>

namespace brk {
	class BRK_API GameTime
	{
	public:
		GameTime();
		~GameTime() = default;

		
		using ClockType = std::chrono::steady_clock;
		using Duration = std::chrono::duration<float>;
		using TimePoint = ClockType::time_point;
		
		void Update();
		void Reset();

		[[nodiscard]] Duration GetDelta() const noexcept { return m_Delta; }
		template <class D>
		[[nodiscard]] D GetDeltaAs() const noexcept
		{
			return std::chrono::duration_cast<D>(m_Delta);
		}

		[[nodiscard]] TimePoint GetStartTime() const noexcept { return m_StartTime; }

		[[nodiscard]] Duration GetElapsed() const noexcept { return m_LastUpdate - m_StartTime; }
		template <class D>
		[[nodiscard]] D GetElapsedAs() const noexcept
		{
			return std::chrono::duration_cast<D>(m_LastUpdate - m_StartTime);
		}

		[[nodiscard]] uint64 GetUpdateCount() const noexcept { return m_UpdateCount; }

	private:
		TimePoint m_StartTime;
		TimePoint m_LastUpdate;
		Duration m_Delta;
		uint64 m_UpdateCount = 0;
	};
} // namespace brk