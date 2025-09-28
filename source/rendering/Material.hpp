#pragma once

#include <PCH.hpp>

#include "Shader.hpp"
#include <asset/Asset.hpp>
#include <asset/AssetRef.hpp>

namespace brk {
	struct AssetMetadata;
	enum class EAssetLoadResult : int8;
} // namespace brk

namespace brk::editor {
	EAssetLoadResult LoadMaterial(IAsset& out_asset, const AssetMetadata& metadata);
}

namespace brk::rdr {
	class BRK_API Material : public IAsset
	{
	public:
		using IAsset::IAsset;
		Material() = default;
		~Material() = default;

		GET_ASSET_TYPE_IMPL(EAssetType::Material);

		[[nodiscard]] const Shader* GetVertexShader() const noexcept { return m_VertShader.Get(); }
		[[nodiscard]] const Shader* GetFragmentShader() const noexcept
		{
			return m_FragShader.Get();
		}

	private:
		friend EAssetLoadResult editor::LoadMaterial(
			IAsset& out_asset,
			const AssetMetadata& metadata);

		AssetRef<Shader> m_VertShader;
		AssetRef<Shader> m_FragShader;
	};
} // namespace brk::rdr