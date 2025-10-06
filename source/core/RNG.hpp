#pragma once

#include <PCH.hpp>

namespace brk {
	/** A fast and reliable PRNG, using xoshiro256ss.
	 */
	class BRK_API RNG
	{
	public:
		using result_type = uint64;

		// The minimum value returned by the generator
		static constexpr result_type min() { return 0; }

		// The maximum value returned by the generator
		static constexpr result_type max() { return ~static_cast<result_type>(0); }

		// Produces the next value in the sequence
		result_type operator()();

		// Reseeds the generator
		void Seed(uint64 s);

		RNG();
		RNG(const uint64 seed);
		RNG(const RNG&) = delete;
		RNG& operator=(const RNG&) = delete;

		~RNG() = default;

	private:
		uint64 m_State[4];
	};
} // namespace brk
