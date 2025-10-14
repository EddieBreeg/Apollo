#pragma once

#include <PCH.hpp>
#include <chrono>

namespace apollo {
	/**
	 * Time management class. Provides information about both frame-times and execution time
	 */
	class GameTime
	{
	public:
		APOLLO_API GameTime();
		~GameTime() = default;

		using ClockType = std::chrono::steady_clock;
		using Duration = std::chrono::duration<float>;
		using TimePoint = ClockType::time_point;

		/**
		 * Called at the end of a frame to compute the delta
		 */
		APOLLO_API void Update();
		/**
		 * Restarts the timer. After this call, GetElapsed() returns 0
		 */
		APOLLO_API void Reset();

		/**
		 * Returns the delta between the two last updates. This corresponds to the duration of the
		 * **previous** frame, or 0 during the first frame
		 */
		[[nodiscard]] Duration GetDelta() const noexcept { return m_Delta; }
		template <class D>
		[[nodiscard]] D GetDeltaAs() const noexcept
		{
			return std::chrono::duration_cast<D>(m_Delta);
		}

		[[nodiscard]] TimePoint GetStartTime() const noexcept { return m_StartTime; }

		/**
		 * The total amount of time elapsed between the object's construction (or the last call to
		 * Reset) and the last update
		 */
		[[nodiscard]] Duration GetElapsed() const noexcept { return m_LastUpdate - m_StartTime; }
		template <class D>
		[[nodiscard]] D GetElapsedAs() const noexcept
		{
			return std::chrono::duration_cast<D>(m_LastUpdate - m_StartTime);
		}

		/**
		 * Returns how many times Update has been called since the object's construction or the last
		 * call to Reset
		 */
		[[nodiscard]] uint64 GetUpdateCount() const noexcept { return m_UpdateCount; }

	private:
		TimePoint m_StartTime;
		TimePoint m_LastUpdate;
		Duration m_Delta;
		uint64 m_UpdateCount = 0;
	};
} // namespace apollo