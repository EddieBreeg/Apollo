#include "AssetRef.hpp"
#include "Asset.hpp"
#include "AssetManager.hpp"

namespace brk {
	void AssetRetainTraits::Increment(IAsset* ptr) noexcept
	{
		if (ptr) [[likely]]
			++ptr->m_RefCount;
	}

	void AssetRetainTraits::Decrement(IAsset* ptr) noexcept
	{
		if (ptr && !--ptr->m_RefCount)
		{
			auto* manager = AssetManager::GetInstance();
			if (manager) [[likely]]
			{
				manager->RequestUnload(ptr);
			}
			else
			{
				delete ptr;
			}
		}
	}

	uint32 AssetRetainTraits::GetCount(const IAsset* ptr) noexcept
	{
		if (ptr) [[likely]]
			return ptr->m_RefCount;
		return 0;
	}
} // namespace brk