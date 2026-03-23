#pragma once

/** \file Errno.hpp */

#include <PCH.hpp>

namespace apollo {
	/**
	 * \brief Returns a human-readable message from an implementation defined error code.
	 *
	 * This needs to exist because WIN32 and POSIX systems can't agree on anything
	 */
	[[nodiscard]] APOLLO_API const char* GetErrnoMessage(int32 code);
} // namespace apollo