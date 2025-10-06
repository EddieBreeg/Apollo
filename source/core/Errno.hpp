#pragma once

#include <PCH.hpp>

namespace brk {
	/**
	 * Returns a human-readable message from an implementation defined error code. This has to exist
	 * because WIN32 and POSIX systems can't agree on anything
	 */
	[[nodiscard]] BRK_API const char* GetErrnoMessage(int32 code);
} // namespace brk