#pragma once

#include <PCH.hpp>
#include <core/Map.hpp>
#include <memory>

namespace std::filesystem {
	class path;
}

namespace brk {
	class IAsset;

	struct AssetMetadata;

	using AssetBankImportFunc =
		bool(const std::filesystem::path& assetFolder, ULIDMap<AssetMetadata>& out_metadataBank);

	using AssetImportFunc = bool(IAsset& out_asset, const AssetMetadata& metadata);
	using AssetConstructor = std::shared_ptr<IAsset>(const ULID& id);
} // namespace brk