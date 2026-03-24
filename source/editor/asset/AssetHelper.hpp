#pragma once

#include <asset/AssetFunctions.hpp>

/** \file AssetHelper.hpp
 * \brief Internal helpers used by the editor for asset management */

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
		static AssetLoadTask LoadAsync(IAsset& out_asset, const AssetMetadata& metadata);
		static void Swap(IAsset& lhs, IAsset& rhs)
		{
			static_cast<A&>(lhs).Swap(static_cast<A&>(rhs));
		}
	};

	/**
	 * Textures have their own specialization, which gets both an immedate and asynchronous load
	 * function
	 */
	template <>
	struct AssetHelper<apollo::rdr::Texture2D>
	{
		static AssetLoadTask LoadAsync(IAsset& out_asset, const AssetMetadata& metadata);

		/// \brief Attempts to load the texture from file immediately. Useful when loading a texture
		/// directly from a file, instead of through the asset manager.
		static bool DoLoad(rdr::Texture2D& out_tex, const AssetMetadata& metadata);

		static void Swap(IAsset& lhs, IAsset& rhs);
	};
} // namespace apollo::editor