#pragma once

#include <asset/AssetFunctions.hpp>

namespace apollo {
	class Scene;
} // namespace apollo

namespace apollo::rdr {
	class FragmentShader;
	class Material;
	class MaterialInstance;
	class Mesh;
	class Texture2D;
	class VertexShader;
} // namespace apollo::rdr

namespace apollo::rdr::txt {
	class FontAtlas;
}

namespace apollo::editor {
	bool ImporteAssetMetadata(
		const std::filesystem::path& assetRootPath,
		ULIDMap<AssetMetadata>& out_metadata);

	template <class A>
	struct AssetHelper
	{
		static EAssetLoadResult Load(IAsset& out_asset, const AssetMetadata& metadata);
	};
} // namespace apollo::editor