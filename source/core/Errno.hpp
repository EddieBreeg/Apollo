#pragma once

#include <PCH.hpp>

namespace brk
{
	[[nodiscard]] BRK_API const char* GetErrnoMessage(int32 code);
}