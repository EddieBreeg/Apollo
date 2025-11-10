#pragma once

#include <asset/AssetFunctions.hpp>

namespace apollo {
	struct AssetManagerSettings;
}

namespace apollo::editor {
	bool ImporteAssetMetadata(
		const std::filesystem::path& assetRootPath,
		ULIDMap<AssetMetadata>& out_metadata);

	EAssetLoadResult LoadTexture2d(IAsset& out_texture, const AssetMetadata& metadata);

	EAssetLoadResult LoadVertexShader(IAsset& out_shader, const AssetMetadata& metadata);
	EAssetLoadResult LoadFragmentShader(IAsset& out_shader, const AssetMetadata& metadata);

	EAssetLoadResult LoadMaterial(IAsset& out_asset, const AssetMetadata& metadata);
	EAssetLoadResult LoadMaterialInstance(IAsset& out_asset, const AssetMetadata& metadata);
	EAssetLoadResult LoadFont(IAsset& out_asset, const AssetMetadata& metadata);
	EAssetLoadResult LoadMesh(IAsset& out_asset, const AssetMetadata& metadata);
	EAssetLoadResult LoadScene(IAsset& out_asset, const AssetMetadata& metadata);
} // namespace apollo::editor