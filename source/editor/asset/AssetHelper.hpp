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
	class AssetManager;

	template <class A>
	struct AssetHelper
	{
		static EAssetLoadResult Load(IAsset& out_asset, const AssetMetadata& metadata);
		static void Swap(IAsset& lhs, IAsset& rhs)
		{
			static_cast<A&>(lhs).Swap(static_cast<A&>(rhs));
		}
	};
} // namespace apollo::editor