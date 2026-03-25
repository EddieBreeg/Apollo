#pragma once

/** \file AssetFunctions.hpp */

#include <PCH.hpp>
#include <core/Map.hpp>

namespace std::filesystem {
	class path;
}

namespace apollo {
	class IAsset;

	struct AssetMetadata;

	/// Result from a load operation
	enum class EAssetLoadResult : int8
	{
		Failure = 0,  /*!< Loading failed */
		Success = 1,  /*!< Loading completed succesfully */
		TryAgain = 2, /*!< Indicates the load request should be attempted again later. This
						 typically occurs when the load task is waiting on another asset to load. */
		Aborted = 3,  /*!<  Loading was aborted for whatever reason */
	};

	struct AssetLoadTask;

	using AssetImportFunc = AssetLoadTask(IAsset& out_asset, const AssetMetadata& metadata);
	using AssetConstructor = IAsset*(const ULID& id);
} // namespace apollo