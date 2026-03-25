#pragma once

#include <PCH.hpp>

/** \file AssetRef.hpp */

#include <core/RetainPtr.hpp>

namespace apollo {
	class IAsset;

	/**
	 * \brief Asset specific RetainTraits class
	 */
	struct APOLLO_API AssetRetainTraits
	{
		static void Increment(IAsset* ptr) noexcept;
		static void Decrement(IAsset* ptr) noexcept;
		static uint32 GetCount(const IAsset* ptr) noexcept;

		static constexpr Retain_t DefaultAction{};
	};

	/**
	 * \brief RetainPtr specialization for asset management
	 */
	template <class A>
	using AssetRef = RetainPtr<A, AssetRetainTraits>;
} // namespace apollo