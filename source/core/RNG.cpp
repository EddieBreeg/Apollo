#include "RNG.hpp"
#include <random>

#define rotl(x, k) (((x) << (k)) | ((x) >> (64 - (k))))

namespace {
	inline uint64 splitmix64(uint64& x)
	{
		x += 0x9e3779b97f4a7c15;
		uint64 z = x;
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
		z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
		return x = (z ^ (z >> 31));
	}

} // namespace

apollo::RNG::RNG()
{
	std::random_device rd;
	uint64 s = (uint64{ rd() } << 32) | rd();
	Seed(s);
}

apollo::RNG::RNG(const uint64 seed)
{
	Seed(seed);
}

void apollo::RNG::Seed(uint64 s)
{
	m_State[0] = splitmix64(s);
	m_State[1] = splitmix64(s);
	m_State[2] = splitmix64(s);
	m_State[3] = splitmix64(s);
}

uint64 apollo::RNG::operator()(void)
{
	const uint64 result = rotl(m_State[1] * 5, 7) * 9;

	const uint64 t = m_State[1] << 17;

	m_State[2] ^= m_State[0];
	m_State[3] ^= m_State[1];
	m_State[1] ^= m_State[2];
	m_State[0] ^= m_State[3];

	m_State[2] ^= t;

	m_State[3] = rotl(m_State[3], 45);

	return result;
}

#undef rotl