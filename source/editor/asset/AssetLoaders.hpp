#pragma once

#include <asset/AssetFunctions.hpp>

namespace brk {
	struct AssetManagerSettings;
}

namespace brk::editor {
	bool ImporteAssetMetadata(
		const std::filesystem::path& assetRootPath,
		ULIDMap<AssetMetadata>& out_metadata);

	EAssetLoadResult ImportTexture2d(IAsset& out_texture, const AssetMetadata& metadata);
	EAssetLoadResult LoadShader(IAsset& out_shader, const AssetMetadata& metadata);
	EAssetLoadResult LoadMaterial(IAsset& out_asset, const AssetMetadata& metadata);	
} // namespace brk::editor