#pragma once

#include <PCH.hpp>
#include <core/Map.hpp>

namespace std::filesystem {
	class path;
}

namespace apollo {
	class IAsset;

	struct AssetMetadata;

	using AssetBankImportFunc =
		bool(const std::filesystem::path& assetFolder, ULIDMap<AssetMetadata>& out_metadataBank);

	enum class EAssetLoadResult : int8
	{
		Failure = 0,
		Success = 1,
		TryAgain = 2, // Indicates the load request should be attempted again later
	};

	using AssetImportFunc = EAssetLoadResult(IAsset& out_asset, const AssetMetadata& metadata);
	using AssetConstructor = IAsset*(const ULID& id);
} // namespace apollo