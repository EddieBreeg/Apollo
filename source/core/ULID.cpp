#include "ULID.hpp"
#include "RNG.hpp"
#include <ctime>

namespace {
	thread_local brk::RNG g_Generator;
}

namespace brk {
	brk::ULID brk::ULID::Generate()
	{
		std::timespec ts;
		std::timespec_get(&ts, TIME_UTC);
		ULID res;
		res.m_Left = static_cast<uint64>(ts.tv_sec) * 1'000 + ts.tv_nsec / 1'000'000;
		res.m_Left = (res.m_Left << 16) | (g_Generator() & 0xffff);
		res.m_Right = g_Generator();

		return res;
	}

} // namespace brk