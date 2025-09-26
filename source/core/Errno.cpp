#include "Errno.hpp"

#include <string.h>

namespace brk {
	const char* GetErrnoMessage(int32 code)
	{
#ifdef _WIN32
		static thread_local char buf[1024];
		strerror_s(buf, code);
		return buf;
#else
		return strerror(code);
#endif
	}
} // namespace brk