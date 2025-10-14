#pragma once

#include <PCH.hpp>

namespace apollo {
	/**
	 * Returns a human-readable message from an implementation defined error code. This has to exist
	 * because WIN32 and POSIX systems can't agree on anything
	 */
	[[nodiscard]] APOLLO_API const char* GetErrnoMessage(int32 code);
} // namespace apollo