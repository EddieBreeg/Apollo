#pragma once

#include <PCH.hpp>

/** \file RNG.hpp */

namespace apollo {
	/**
	 * \brief A fast and reliable PRNG, used mostly for things like ULID generation.
	 * \details This implementation uses the
	 * [xorshiro256**](https://en.wikipedia.org/wiki/Xorshift#xoshiro256**) algorithm to generate
	 * the values. Seeding is done using the SplitMix64 function.
	 */
	class APOLLO_API RNG
	{
	public:
		using result_type = uint64;

		/// The minimum value returned by the generator
		static constexpr result_type min() { return 0; }

		/// The maximum value returned by the generator
		static constexpr result_type max() { return ~static_cast<result_type>(0); }

		/// Produces the next value in the sequence
		result_type operator()();

		/// Reseeds the generator
		void Seed(uint64 s);

		/*! \brief Initializes the internal state
		 * The seed is generated using <b>std::random_device</b>. This generator is
		 * non-deterministic on all platforms I know of.
		 */
		RNG();
		/// Initializes the state using the provided seed. All values are allowed.
		RNG(const uint64 seed);
		RNG(const RNG&) = delete;
		RNG& operator=(const RNG&) = delete;

		~RNG() = default;

	private:
		uint64 m_State[4];
	};
} // namespace apollo
