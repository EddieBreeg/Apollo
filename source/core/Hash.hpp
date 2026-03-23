#pragma once

#include <PCH.hpp>
#include <bit>

/**
 * \file Hash.hpp

 * \requirement Hasher
 * A type H is a Hasher type if given:
 - k an object of type `const Key&`
 - h an object of type `H` or `const H`

 `h(k)` produces a value which is convertible to a uint64. The result should be the same after every
 invocation.

 \requirement Hashable
 A type T is hashable if \ref apollo::Hash<T> "Hash<T>" meets the requirements of Hasher for key
 type T.
 */

namespace apollo {
	/**
	 * \brief Generic object hasher.

	 This base template gets specialized for all types which need to be hashed.
	 */
	template <class T>
	struct Hash;

	/**
	* \verifies Hasher
	 \tparam H: The hasher type to test
	 \tparam K: The key type
	 */
	template <class H, class K>
	concept Hasher = requires(H hasher, const K& key)
	{
		{ hasher(key) }->std::convertible_to<uint64>;
	};

	/**
	* \brief Tests whether a given type is hashable.

	\tparam T: The key type to test

	Evaluates to true if \ref apollo::Hash<T> "Hash<T>" meets the requirements of Hasher for type T.
	\verifies Hashable
	 */
	template <class T>
	concept Hashable = Hasher<Hash<T>, T>;

	/**
	 * \brief Integer hash implementation
	 * \satisfies Hasher
	 */
	template <std::integral Int>
	struct Hash<Int>
	{
		[[nodiscard]] constexpr auto operator()(Int val) const noexcept -> std::make_unsigned_t<Int>
		{
			return static_cast<std::make_unsigned_t<Int>>(val);
		}
	};

	/**
	 * \brief float hash implementation
	 * \satisfies Hasher
	 */
	template <>
	struct Hash<float>
	{
		[[nodiscard]] constexpr uint32 operator()(float val) const noexcept
		{
			return std::bit_cast<uint32>(val);
		}
	};
	/**
	 * \brief double hash implementation
	 * \satisfies Hasher
	 */
	template <>
	struct Hash<double>
	{
		[[nodiscard]] constexpr uint64 operator()(double val) const noexcept
		{
			return std::bit_cast<uint64>(val);
		}
	};

	/**
	 * \brief Pointer hash implementation
	 \details This specialization simply casts the pointer to an integer value, and does not attempt
	 to dereference it
	 * \satisfies Hasher
	 */
	template <class T>
	struct Hash<T*>
	{
		[[nodiscard]] uintptr_t operator()(T* const ptr) const noexcept { return uintptr_t(ptr); }
	};

	/**
	 * \name Hash combination primitives
	 * \brief The HashCombine primitive computes a single hash from multiple keys.
	 * @{
	 */

	/**
	 * \param seed: The initial hash value. This is typically returned from a specialization of Hash
	 * \param val: The values to combine with the seed. Each one will be hashed using the default
	 * corresponding specialization of Hash
	 * \verifies Hashable
	 */
	template <class... K>
	[[nodiscard]] constexpr uint64 HashCombine(uint64 seed, K&&... val) noexcept(
		(noexcept(Hash<std::decay_t<K>>{}(std::forward<K>(val))) && ...))
		requires(Hashable<std::decay_t<K>>&&...)
	{
		((seed ^= Hash<std::decay_t<K>>{}(std::forward<K>(val)) + 0x90e14f3da127aed8 + (seed << 6) +
				  (seed >> 2)),
		 ...);
		return seed;
	}

	/**
	 * \param seed: The initial hash value. This is typically returned from a specialization of Hash
	 * \param hasher: The hasher object to use for the computations
	 * \param val: The values to combine with the seed. Each one will be hashed using the default
	 * corresponding specialization of Hash
	 * \verifies Hashable
	 * \details This overload does the same thing as the first one, but with a user provided hasher
	 * object.
	 */
	template <class H, class... K>
	[[nodiscard]] constexpr uint64 HashCombine(uint64 seed, H&& hasher, K&&... val) noexcept(
		(noexcept(hasher(val)) && ...)) requires(Hasher<H, std::decay_t<K>>&&...)
	{
		((seed ^= hasher(std::forward<K>(val)) + 0x90e14f3da127aed8 + (seed << 6) + (seed >> 2)),
		 ...);
		return seed;
	}

	/*! @} */
} // namespace apollo