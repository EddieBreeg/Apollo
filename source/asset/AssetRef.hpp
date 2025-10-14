#pragma once

#include <PCH.hpp>

#include <core/RetainPtr.hpp>

namespace apollo {
	class IAsset;

	struct APOLLO_API AssetRetainTraits
	{
		static void Increment(IAsset* ptr) noexcept;
		static void Decrement(IAsset* ptr) noexcept;
		static uint32 GetCount(const IAsset* ptr) noexcept;

		static constexpr Retain_t DefaultAction{};
	};

	template <class A>
	using AssetRef = RetainPtr<A, AssetRetainTraits>;
} // namespace apollo