#include "AssetRef.hpp"
#include "Asset.hpp"

namespace brk {
	void AssetRetainTraits::Increment(IAsset* ptr) noexcept
	{
		if (ptr) [[likely]]
			++ptr->m_RefCount;
	}

	void AssetRetainTraits::Decrement(IAsset* ptr) noexcept
	{
		if (!ptr) [[unlikely]]
			return;

		if (!--ptr->m_RefCount)
			delete ptr;
	}

	uint32 AssetRetainTraits::GetCount(const IAsset* ptr) noexcept
	{
		if (ptr) [[likely]]
			return ptr->m_RefCount;
		return 0;
	}
} // namespace brk