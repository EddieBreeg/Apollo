#pragma once

#include <unordered_map>

/**
 * \file Map.hpp
 * \brief Hashmap typedefs
 */

namespace apollo {
	class ULID;

	template <class>
	struct Hash;
	struct StringHash;

	/**
	 * \brief std::unordered_map specialization using apollo::Hash

	 This is typically the specialization used all across the engine.
	 */
	template <class K, class T>
	using HashMap = std::unordered_map<K, T, Hash<K>>;

	template <class T>
	using ULIDMap = HashMap<ULID, T>;

	template <class T>
	using StringHashMap = HashMap<StringHash, T>;
} // namespace apollo