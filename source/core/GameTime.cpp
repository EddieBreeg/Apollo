#include "GameTime.hpp"

namespace apollo {
	GameTime::GameTime()
		: m_StartTime(ClockType::now())
		, m_LastUpdate(m_StartTime)
	{}

	void GameTime::Update()
	{
		const auto t = ClockType::now();
		m_Delta = t - m_LastUpdate;
		m_LastUpdate = t;
		++m_UpdateCount;
	}

	void GameTime::Reset()
	{
		m_StartTime = m_LastUpdate = ClockType::now();
		m_Delta = Duration{};
		m_UpdateCount = 0;
	}
} // namespace apollo