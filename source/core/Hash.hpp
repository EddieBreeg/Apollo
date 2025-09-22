#pragma once

#include <PCH.hpp>
#include <bit>

namespace brk {
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

	template <class T>
	struct Hash<T*>
	{
		[[nodiscard]] uintptr_t operator()(T* const ptr) const noexcept { return uintptr_t(ptr); }
	};

	template <class... K>
	[[nodiscard]] constexpr uint64 HashCombine(uint64 seed, K&&... val)
		requires(Hashable<std::decay_t<K>>&&...)
	{
		((seed ^= Hash<std::decay_t<K>>{}(std::forward<K>(val)) + 0x90e14f3da127aed8 + (seed << 6) +
				  (seed >> 2)),
		 ...);
		return seed;
	}

	template <class H, class... K>
	[[nodiscard]] constexpr uint64 HashCombine(uint64 seed, H&& hasher, K&&... val) noexcept(
		(noexcept(hasher(val)) && ...)) requires(Hasher<H, std::decay_t<K>>&&...)
	{
		((seed ^= hasher(std::forward<K>(val)) + 0x90e14f3da127aed8 + (seed << 6) + (seed >> 2)),
		 ...);
		return seed;
	}

	struct StringHash
	{
		explicit constexpr StringHash(const char* str) noexcept
		{
			while (const char c = *str++)
			{
				m_Hash += c;
				m_Hash += m_Hash << 10;
				m_Hash ^= m_Hash >> 6;
			}
			m_Hash *= 0x68f96d6d;
			m_Hash ^= m_Hash >> 7;
			m_Hash *= m_Hash << 3;
			m_Hash ^= m_Hash >> 11;
			m_Hash += m_Hash << 15;
		}
		constexpr StringHash(const uint32 h) noexcept
			: m_Hash(h)
		{}

		[[nodiscard]] constexpr operator uint32() const noexcept { return m_Hash; }

	private:
		uint32 m_Hash = 0;
	};

	template <>
	struct Hash<StringHash>
	{
		[[nodiscard]] constexpr uint32 operator()(StringHash h) const noexcept { return h; }
	};

	namespace string_hash_literals {
		consteval StringHash operator""_strh(const char* str) noexcept
		{
			return StringHash{ str };
		}
	} // namespace string_hash_literals
} // namespace brk