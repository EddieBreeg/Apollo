#pragma once

#include <unordered_map>

/**
 * Just some hashmap shortcuts
 */

namespace apollo {
	class ULID;

	template <class>
	struct Hash;
	struct StringHash;

	template <class K, class T>
	using HashMap = std::unordered_map<K, T, Hash<K>>;

	template <class T>
	using ULIDMap = HashMap<ULID, T>;

	template <class T>
	using StringHashMap = HashMap<StringHash, T>;
} // namespace apollo