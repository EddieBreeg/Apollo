#pragma once

#include <asset/Importer.hpp>

namespace brk {
	struct AssetManagerSettings;
}

namespace brk::editor {
	bool ImporteAssetMetadata(
		const std::filesystem::path& assetRootPath,
		ULIDMap<AssetMetadata>& out_metadata);

	bool ImportTexture2d(IAsset& out_texture, const AssetMetadata& metadata);
	bool LoadShader(IAsset& out_shader, const AssetMetadata& metadata);
}