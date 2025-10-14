#include "TypeInfo.hpp"
#include <atomic>

namespace {
	std::atomic_uint32_t g_Index = 0;
}

namespace apollo {
	uint32 _internal::DefaultTypeIndexGenerator::GetNext() noexcept
	{
		return g_Index++;
	}
} // namespace apollo