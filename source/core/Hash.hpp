#pragma once

#include <PCH.hpp>
#include <bit>

namespace apollo {
	/**
	 * Generic object hasher. This should have a specialisation for any type T which needs to be
	 * hashed
	 */
	template <class T>
	struct Hash;

	// Evaluates to true if H can be used to hash a value of type K
	template <class H, class K>
	concept Hasher = requires(H hasher, const K& key)
	{
		{ hasher(key) }->std::convertible_to<uint64>;
	};

	template <class T>
	concept Hashable = requires(const T& val, Hash<T> h)
	{
		{ h(val) }->std::convertible_to<uint64>;
	};

	/**
	 * Integer hash implementation
	 */
	template <std::integral Int>
	struct Hash<Int>
	{
		[[nodiscard]] constexpr auto operator()(Int val) const noexcept -> std::make_unsigned_t<Int>
		{
			return static_cast<std::make_unsigned_t<Int>>(val);
		}
	};

	template <>
	struct Hash<float>
	{
		[[nodiscard]] constexpr uint32 operator()(float val) const noexcept
		{
			return std::bit_cast<uint32>(val);
		}
	};
	template <>
	struct Hash<double>
	{
		[[nodiscard]] constexpr uint64 operator()(double val) const noexcept
		{
			return std::bit_cast<uint64>(val);
		}
	};

	/**
	 *  Pointer implementation.
	 */
	template <class T>
	struct Hash<T*>
	{
		/**
		 * This casts the pointer to an integer value, and does not attempt to dereference it
		 */
		[[nodiscard]] uintptr_t operator()(T* const ptr) const noexcept { return uintptr_t(ptr); }
	};

	/**
	 * Hash combination primitive. Useful to hash multiple objects together
	 * \param  seed: The initial hash value. This is typically returned from a specialisation of Hash
	 * \param val: The values to combine with the seed. Each one will be hashed using the default
	 * corresponding specialisation of Hash
	 */
	template <class... K>
	[[nodiscard]] constexpr uint64 HashCombine(uint64 seed, K&&... val)
		requires(Hashable<std::decay_t<K>>&&...)
	{
		((seed ^= Hash<std::decay_t<K>>{}(std::forward<K>(val)) + 0x90e14f3da127aed8 + (seed << 6) +
				  (seed >> 2)),
		 ...);
		return seed;
	}

	/**
	 * This overload does the same thing as the the one above, except the user can specify a hasher
	 * object, which will be used to hash every value in the parameter pack.
	 */
	template <class H, class... K>
	[[nodiscard]] constexpr uint64 HashCombine(uint64 seed, H&& hasher, K&&... val) noexcept(
		(noexcept(hasher(val)) && ...)) requires(Hasher<H, std::decay_t<K>>&&...)
	{
		((seed ^= hasher(std::forward<K>(val)) + 0x90e14f3da127aed8 + (seed << 6) + (seed >> 2)),
		 ...);
		return seed;
	}
} // namespace apollo